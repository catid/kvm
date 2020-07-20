// Copyright 2020 Christopher A. Taylor

/*
    Image format conversion
*/

#pragma once

#include "kvm_core.hpp"

namespace kvm {


//------------------------------------------------------------------------------
// Convert

// Convert from JPEG colorspace YUV422 -> YUV420 BT.709
void Convert_JPEG_YUV422_To_BT709_YUV420(uint8_t* planes[3], int w, int stride[3], int h);


} // namespace kvm
