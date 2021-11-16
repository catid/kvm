// Copyright 2020 Christopher A. Taylor

/*
    Memory pool allocator for video frame images in various formats
*/

#pragma once

#include "kvm_core.hpp"

#include <memory>
#include <vector>
#include <mutex>

namespace kvm {


//------------------------------------------------------------------------------
// Constants

enum class PixelFormat
{
    Invalid,

    // Compressed formats:
    JPEG,       // JPEG compressed images

    // Raw formats:
    YUV420P,    // 4:2:0 three planes: Y, U, V
    YUV422P,    // 4:2:2 three planes: Y, U, V
    YUYV,       // 4:2:2 single plane
    NV12,       // 4:2:0 two planes: Y, interleaved UV
    RGB24,
};


//------------------------------------------------------------------------------
// Frame

struct Frame
{
    Frame();
    ~Frame();

    int Width = 0;
    int Height = 0;
    PixelFormat Format = PixelFormat::YUV422P;
    int AllocatedBytes = 0;

    uint8_t* Planes[3];
};


//------------------------------------------------------------------------------
// FramePool

class FramePool
{
public:
    std::shared_ptr<Frame> Allocate(int w, int h, PixelFormat format);

    void Release(const std::shared_ptr<Frame>& frame);

protected:
    std::mutex Lock;
    std::vector<std::shared_ptr<Frame>> Freed;
};


} // namespace kvm
