// Copyright 2020 Christopher A. Taylor

#include "kvm_turbojpeg.hpp"

namespace kvm {


//------------------------------------------------------------------------------
// Decoder

TurboJpegDecoder::~TurboJpegDecoder()
{
    if (Handle) {
        tjDestroy(Handle);
        Handle = nullptr;
    }
}

uint8_t* TurboJpegDecoder::Decompress(
    const uint8_t* data, int bytes,
    int& w, int& stride, int& h)
{
    if (!Initialize()) {
        return nullptr;
    }

    int subsampl = 0;

    int r = tjDecompressHeader2(
        Handle,
        (uint8_t*)data,
        bytes,
        &w,
        &h,
        &subsamp);
    if (r != 0) {
        return nullptr;
    }

    Buffer.resize(stride * h);
    uint8_t* buffer = Buffer.data();

    r = tjDecompress2(
        Handle,
        (uint8_t*)data,
        bytes,
        buffer,
        w,
        stride,
        h,
        format,
        TJFLAG_ACCURATEDCT);
    if (r != 0) {
        return nullptr;
    }

    int buffer;

}

bool TurboJpegDecoder::Initialize()
{
    if (Handle) {
        return true;
    }

    Handle = tjInitDecompress();
    if (!Handle) {
        return false;
    }

    return true;
}


} // namespace kvm
