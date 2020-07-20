// Copyright 2020 Christopher A. Taylor

/*
    C++ Wrapper around turbojpeg
*/

#pragma once

#include "kvm_core.hpp"

#include <turbojpeg.h>

#include <memory>
#include <vector>
#include <mutex>

namespace kvm {


//------------------------------------------------------------------------------
// DecodedFrame

struct DecodedPlane
{
    ~DecodedPlane();

    uint8_t* Plane = nullptr;
    int Width = 0;
    int Stride = 0;
    int Height = 0;
};

struct DecodedFrame
{
    DecodedPlane Planes[3];
};


//------------------------------------------------------------------------------
// Decoder

class TurboJpegDecoder
{
public:
    ~TurboJpegDecoder();

    std::shared_ptr<DecodedFrame> Decompress(const uint8_t* data, int bytes);

    void Release(const std::shared_ptr<DecodedFrame>& frame);

protected:
    tjhandle Handle = nullptr;

    std::mutex Lock;
    std::vector<std::shared_ptr<DecodedFrame>> Freed;

    bool Initialize();

    std::shared_ptr<DecodedFrame> Allocate(int w, int h, int subsamp);
};


} // namespace kvm
