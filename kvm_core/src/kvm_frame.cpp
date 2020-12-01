// Copyright 2020 Christopher A. Taylor

#include "kvm_frame.hpp"
#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Frame");


//------------------------------------------------------------------------------
// Frame

Frame::Frame()
{
    Planes[0] = nullptr;
}

Frame::~Frame()
{
    AlignedFree(Planes[0]);
    Planes[0] = nullptr;
}


//------------------------------------------------------------------------------
// FramePool

static int RoundUp32(int x)
{
    return (x + 31) & ~31;
}

static int RoundUp16(int x)
{
    return (x + 15) & ~15;
}

std::shared_ptr<Frame> FramePool::Allocate(int w, int h, PixelFormat format)
{
    // Designed for ingest into MMAL encoder
    w = RoundUp32(w);
    h = RoundUp16(h);

    // Check if we can use one from the pool:
    {
        std::lock_guard<std::mutex> locker(Lock);
        if (!Freed.empty()) {
            auto frame = Freed.back();
            Freed.pop_back();
            return frame;
        }
    }

    auto frame = std::make_shared<Frame>();
    frame->Width = w;
    frame->Height = h;
    frame->Format = format;

    if (format == PixelFormat::RGB24) {
        frame->AllocatedBytes = w * h * 3;
        uint8_t* data = AlignedAllocate(frame->AllocatedBytes);
        if (!data) {
            Logger.Error("Out of memory: Unable to allocate more raw frames");
            return nullptr;
        }
        frame->Planes[0] = data;
        frame->Planes[1] = frame->Planes[2] = nullptr;
        return frame;
    }

    int y_plane_bytes = w * h, uv_plane_bytes = 0;
    if (format == PixelFormat::YUV420P) {
        uv_plane_bytes = y_plane_bytes / 4;
    }
    else if (format == PixelFormat::YUV422P) {
        uv_plane_bytes = y_plane_bytes / 2;
    }
    else {
        Logger.Error("FIXME: Unsupported format");
        return nullptr;
    }

    frame->AllocatedBytes = y_plane_bytes + uv_plane_bytes * 2;
    uint8_t* data = AlignedAllocate(frame->AllocatedBytes);
    if (!data) {
        Logger.Error("Out of memory: Unable to allocate more raw frames");
        return nullptr;
    }
    frame->Planes[0] = data;
    frame->Planes[1] = frame->Planes[0] + y_plane_bytes;
    frame->Planes[2] = frame->Planes[1] + uv_plane_bytes;
    return frame;
}

void FramePool::Release(const std::shared_ptr<Frame>& frame)
{
    std::lock_guard<std::mutex> locker(Lock);
    Freed.push_back(frame);
}


} // namespace kvm
