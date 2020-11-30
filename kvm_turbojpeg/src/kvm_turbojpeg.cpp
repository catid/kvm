// Copyright 2020 Christopher A. Taylor

#include "kvm_turbojpeg.hpp"
#include "kvm_logger.hpp"

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
*/
static void ConvertYuv422toYuv420(const uint8_t* src, uint8_t* dest, int w, int h)
{
#if 1 // Reference version
    const int dest_height = h / 2;
    const int dest_width = w / 2;

    const uint8_t* src1 = src + dest_width;

    for (int y = 0; y < dest_height; ++y) {
        for (int x = 0; x < dest_width; ++x) {
            dest[x] = static_cast<uint8_t>( ((unsigned)src[x] + (unsigned)src1[x] + 1) >> 1 );
        }

        // TBD: Assumes width = stride
        dest += dest_width;
        src += dest_width * 2;
        src1 += dest_width * 2;
    }
#endif
}


//------------------------------------------------------------------------------
// Decoder

TurboJpegDecoder::~TurboJpegDecoder()
{
    if (Handle) {
        tjDestroy(Handle);
        Handle = nullptr;
    }
}

std::shared_ptr<Frame> TurboJpegDecoder::Decompress(const uint8_t* data, int bytes)
{
    if (!Initialize()) {
        return nullptr;
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

#if 0 // Faster but more complex:

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
        ConvertYuv422toYuv420(Yuv422TempFrame->Planes[1], frame->Planes[1], w, h);
        ConvertYuv422toYuv420(Yuv422TempFrame->Planes[2], frame->Planes[2], w, h);
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

bool TurboJpegDecoder::Initialize()
{
    if (Handle) {
        return true;
    }

    Handle = tjInitDecompress();
    if (!Handle) {
        Logger.Error("tjInitDecompress failed");
        return false;
    }

    return true;
}


} // namespace kvm
