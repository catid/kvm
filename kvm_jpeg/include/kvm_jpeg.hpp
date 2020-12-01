// Copyright 2020 Christopher A. Taylor

/*
    C++ Wrapper around turbojpeg and the brcmjpeg hw decoder

    References:
    [1] https://github.com/libjpeg-turbo/libjpeg-turbo/blob/master/turbojpeg.h
*/

#pragma once

#include "kvm_core.hpp"
#include "kvm_frame.hpp"

#include "brcmjpeg.h"

#include <turbojpeg.h>

namespace kvm {


//------------------------------------------------------------------------------
// JpegDecoder

class JpegDecoder
{
public:
    ~JpegDecoder()
    {
        Shutdown();
    }

    std::shared_ptr<Frame> Decompress(const uint8_t* data, int bytes);

    void Release(const std::shared_ptr<Frame>& frame)
    {
        Pool.Release(frame);
    }

protected:
    // TurboJpeg decoder
    tjhandle Handle = nullptr;

    // BRCM hw decoder
    BRCMJPEG_T* BroadcomDecoder = nullptr;

    FramePool Pool;

    std::shared_ptr<Frame> Yuv422TempFrame;

    bool Initialize();
    void Shutdown();
};


} // namespace kvm
