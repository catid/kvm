// Copyright 2020 Christopher A. Taylor

/*
    C++ Wrapper around turbojpeg
*/

#pragma once

#include <turbojpeg.h>

namespace kvm {


//------------------------------------------------------------------------------
// Decoder

class TurboJpegDecoder
{
    ~TurboJpegDecoder();

    uint8_t* Decompress(
        const uint8_t* data, int bytes,
        int& w, int& stride, int& h);

protected:
    bool Initialize();

    tjhandle Handle = nullptr;
    std::vector<uint8_t> Buffer;
};


} // namespace kvm
