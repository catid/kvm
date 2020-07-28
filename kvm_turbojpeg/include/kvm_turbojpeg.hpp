// Copyright 2020 Christopher A. Taylor

/*
    C++ Wrapper around turbojpeg

    Reference:
    https://github.com/libjpeg-turbo/libjpeg-turbo/blob/master/turbojpeg.h
*/

#pragma once

#include "kvm_core.hpp"
#include "kvm_frame.hpp"

#include <turbojpeg.h>

namespace kvm {


//------------------------------------------------------------------------------
// Decoder

class TurboJpegDecoder
{
public:
    ~TurboJpegDecoder();

    std::shared_ptr<Frame> Decompress(const uint8_t* data, int bytes);

    void Release(const std::shared_ptr<Frame>& frame)
    {
        Pool.Release(frame);
    }

protected:
    tjhandle Handle = nullptr;
    FramePool Pool;

    bool Initialize();
};


} // namespace kvm
