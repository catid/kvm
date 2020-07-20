// Copyright 2020 Christopher A. Taylor

#include "kvm_turbojpeg.hpp"
#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Capture");


//------------------------------------------------------------------------------
// DecodedFrame

DecodedPlane::~DecodedPlane()
{
    AlignedFree(Plane);
    Plane = nullptr;
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

std::shared_ptr<DecodedFrame> TurboJpegDecoder::Decompress(const uint8_t* data, int bytes)
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
        Logger.Error("tjDecompressHeader2 failed");
        return nullptr;
    }

    auto frame = Allocate(w, h, subsamp);
    if (!frame) {
        Logger.Error("Allocate failed");
        return nullptr;
    }

    uint8_t* dstPlanes[3] = {
        frame->Planes[0].Plane,
        frame->Planes[1].Plane,
        frame->Planes[2].Plane
    };
    int strides[3] = {
        frame->Planes[0].Stride,
        frame->Planes[1].Stride,
        frame->Planes[2].Stride
    };

    r = tjDecompressToYUVPlanes(
        Handle,
        (uint8_t*)data,
        bytes,
        dstPlanes,
        w,
        strides,
        h,
        TJFLAG_ACCURATEDCT);
    if (r != 0) {
        Logger.Error("tjDecompressToYUVPlanes failed");
        return nullptr;
    }

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

static int GetStride(int w)
{
    return (w + 31) & ~31;
}

std::shared_ptr<DecodedFrame> TurboJpegDecoder::Allocate(int w, int h, int subsamp)
{
    if (subsamp != TJSAMP_422) {
        Logger.Error("Unsupported subsamp: ", subsamp);
        return nullptr;
    }

    // Check if we can use one from the pool:
    {
        std::lock_guard<std::mutex> locker(Lock);
        if (!Freed.empty()) {
            auto frame = Freed.back();
            Freed.pop_back();
            return frame;
        }
    }

    auto frame = std::make_shared<DecodedFrame>();
    for (int i = 0; i < 3; ++i) {
        auto& plane = frame->Planes[i];
        if (i == 0) {
            plane.Width = w;
        }
        else {
            // JPEG colorspace YUV422 format
            // For each 2x1 Y you get one CbCr
            plane.Width = w/2;
        }
        plane.Height = h;
        plane.Stride = GetStride(plane.Width);
        plane.Plane = AlignedAllocate(plane.Stride * plane.Height);
    }
    return frame;
}

void TurboJpegDecoder::Release(const std::shared_ptr<DecodedFrame>& frame)
{
    std::lock_guard<std::mutex> locker(Lock);
    Freed.push_back(frame);
}


} // namespace kvm
