// Copyright 2020 Christopher A. Taylor

#include "kvm_jpeg.hpp"
#include "kvm_logger.hpp"

#include <omp.h>
#include <arm_neon.h>

namespace kvm {

static logger::Channel Logger("Capture");


//------------------------------------------------------------------------------
// Convert YUV422 to YUV420

/*
    Convert a single chroma plane of YUV422 -> YUV420

    Provide pointer to start of source and destination planes.
    Provide width and height of the Y plane.

    This can be done two ways:
    (1) Discard every other row (fast)
    (2) Blend consecutive rows together (not much slower, avoids missing colors)

    We do option (2) and use ARM NEON to speed up the calculation.
*/
static void ConvertYuv422toYuv420(const uint8_t* src, uint8_t* dest, int w, int h)
{
    const int dest_height = h / 2;
    const int dest_width = w / 2;

    const uint8_t* src1 = src + dest_width;

    for (int y = 0; y < dest_height; ++y) {
        // Note: Assumes MMAL padded inputs (16 bytes)
#ifdef __ARM_NEON__ // ARM NEON optimized version:
        // Note: Raspbian is a 32-bit OS with 32-bit tools and cannot use ARM NEON by default
        for (int x = 0; x < dest_width; x += 16) {
            vst1_u8(dest + x, vshrn_n_u16(vaddl_u8(vld1_u8(src + x), vld1_u8(src1 + x)), 1));
            vst1_u8(dest + x + 8, vshrn_n_u16(vaddl_u8(vld1_u8(src + x + 8), vld1_u8(src1 + x + 8)), 1));
        }
#else // Reference version:
        for (int x = 0; x < dest_width; x += 8) {
            dest[x] = static_cast<uint8_t>( ((unsigned)src[x] + (unsigned)src1[x] + 1) >> 1 );
            dest[x+1] = static_cast<uint8_t>( ((unsigned)src[x+1] + (unsigned)src1[x+1] + 1) >> 1 );
            dest[x+2] = static_cast<uint8_t>( ((unsigned)src[x+2] + (unsigned)src1[x+2] + 1) >> 1 );
            dest[x+3] = static_cast<uint8_t>( ((unsigned)src[x+3] + (unsigned)src1[x+3] + 1) >> 1 );
            dest[x+4] = static_cast<uint8_t>( ((unsigned)src[x+4] + (unsigned)src1[x+4] + 1) >> 1 );
            dest[x+5] = static_cast<uint8_t>( ((unsigned)src[x+5] + (unsigned)src1[x+5] + 1) >> 1 );
            dest[x+6] = static_cast<uint8_t>( ((unsigned)src[x+6] + (unsigned)src1[x+6] + 1) >> 1 );
            dest[x+7] = static_cast<uint8_t>( ((unsigned)src[x+7] + (unsigned)src1[x+7] + 1) >> 1 );
        }
#endif

        // TBD: Assumes width = stride
        dest += dest_width;
        src += dest_width * 2;
        src1 += dest_width * 2;
    }
}


//------------------------------------------------------------------------------
// JpegDecoder

bool JpegDecoder::Initialize()
{
    BRCMJPEG_STATUS_T status = brcmjpeg_create(BRCMJPEG_TYPE_DECODER, &BroadcomDecoder);
    if (status != BRCMJPEG_SUCCESS || !BroadcomDecoder) {
        Logger.Warn("brcmjpeg_create failed: status=", status, " Cannot use hardware JPEG decoder");
    }

    Handle = tjInitDecompress();
    if (!Handle) {
        Logger.Error("tjInitDecompress failed");
        return false;
    }

    return true;
}

void JpegDecoder::Shutdown()
{
    if (Handle) {
        tjDestroy(Handle);
        Handle = nullptr;
    }

    if (BroadcomDecoder) {
        brcmjpeg_release(BroadcomDecoder);
        BroadcomDecoder = nullptr;
    }
}

std::shared_ptr<Frame> JpegDecoder::Decompress(const uint8_t* data, int bytes)
{
    // If TurboJpeg handle is not initialized yet:
    if (!Handle) {
        if (!Initialize()) {
            return nullptr;
        }
    }

    int w = 0, h = 0, subsamp = 0;
    int r = tjDecompressHeader2(
        Handle,
        (uint8_t*)data,
        bytes,
        &w,
        &h,
        &subsamp);
    if (r != 0) {
        Logger.Error("tjDecompressHeader2 failed: r=", r, " err=", tjGetErrorStr());
        return nullptr;
    }

    /*
        Benchmark results on a selected desktop image:

        YUV422 + Conversion to YUV420: 16.5 milliseconds (avg)
        YUV420 Video Encode: 24 milliseconds (avg)

        TurboJPEG Conversion to RGB: 17.5 milliseconds
        RGB Video Encode: 28.5 milliseconds (avg)
    */
#if 1 // Faster but more complex:

    PixelFormat format;
    if (subsamp == TJSAMP_420) {
        format = PixelFormat::YUV420P;
    }
    else if (subsamp == TJSAMP_422) {
        // Note: We will convert to YUV420 on CPU below
        format = PixelFormat::YUV420P;
        if (!Yuv422TempFrame) {
            Yuv422TempFrame = Pool.Allocate(w, h, PixelFormat::YUV422P);
            if (!Yuv422TempFrame) {
                return nullptr;
            }
        }
    }
    else {
        Logger.Error("Unsupported subsamp: ", subsamp);
        return nullptr;
    }

    auto frame = Pool.Allocate(w, h, format);
    if (!frame) {
        Logger.Error("Pool.Allocate failed");
        return nullptr;
    }

    int strides[3] = {
        w,
        w/2,
        w/2
    };

    uint8_t* planes[3] = {
        frame->Planes[0],
        frame->Planes[1],
        frame->Planes[2]
    };

    if (subsamp == TJSAMP_422) {
        // Use larger temporary space instead
        planes[1] = Yuv422TempFrame->Planes[1];
        planes[2] = Yuv422TempFrame->Planes[2];
    }

    r = tjDecompressToYUVPlanes(
        Handle,
        (uint8_t*)data,
        bytes,
        planes,
        w,
        strides,
        h,
        TJFLAG_ACCURATEDCT);
    if (r != 0) {
        Logger.Error("tjDecompressToYUVPlanes failed: r=", r, " err=", tjGetErrorStr(), " inlen=", bytes, " w=", w, " h=", h);
        return nullptr;
    }

    if (subsamp == TJSAMP_422) {
        // Note: Seems to require "num_threads(2)" to actually run operations in parallel.
        // When they run in parallel, this operation takes about 1.5 milliseconds.
#pragma omp parallel for num_threads(2)
        for (int i = 1; i <= 2; ++i) {
            ConvertYuv422toYuv420(Yuv422TempFrame->Planes[i], frame->Planes[i], w, h);
        }
    }

#else // RGB:

    auto frame = Pool.Allocate(w, h, PixelFormat::RGB24);
    if (!frame) {
        Logger.Error("Allocate failed");
        return nullptr;
    }

    r = tjDecompress2(
        Handle,
        (uint8_t*)data,
        bytes,
        frame->Planes[0],
        w,
        w*3,
        h,
        TJPF_RGB,
        TJFLAG_ACCURATEDCT);
    if (r != 0) {
        Logger.Error("tjDecompressToYUVPlanes failed: r=", r, " err=", tjGetErrorStr(), " inlen=", bytes, " w=", w, " h=", h);
        return nullptr;
    }

#endif

    return frame;
}


} // namespace kvm
