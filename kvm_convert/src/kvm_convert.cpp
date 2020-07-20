// Copyright 2020 Christopher A. Taylor

#include "kvm_convert.hpp"
#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Convert");


//------------------------------------------------------------------------------
// Convert

/*
    JPEG YUV422 is full range 0..255 in each component, using BT.601.

    BT.601 RGB -> YUV:
        kYr = 0.299
        kYg = 0.587
        kYb = 0.114
        kCb = 1.0 / 1.772
        kCr = 1.0 / 1.402

        Y = r * kYr + g * kYg + b * kYb
        U = (b - Y) * kCb
        V = (r - Y) * kCr

    Y’ is scaled to [0...255] and Cb/Cr are scaled to [-128...128] and then clipped to [-128...127]

    BT.709 RGB -> YUV:
        kYr = 0.2126
        kYg = 0.7152
        kYb = 0.0722
        kCb = 1.0 / 1.8556
        kCr = 1.0 / 1.5748

        Y = r * kYr + g * kYg + b * kYb
        U = (b - Y) * kCb
        V = (r - Y) * kCr

    scale = 255 (if r,g,b go to 255)
    Y' = (uint8_t)( Y * 219 * scale + 0.5 ) + 16
    U' = (uint8_t)( round(224 * U * scale) + 128 )
*/
void Convert_JPEG_YUV422_To_BT709_YUV420(uint8_t* planes[3], int w, int stride[3], int h)
{
    // FIXME
}


} // namespace kvm
