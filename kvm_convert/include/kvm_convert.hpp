// Copyright 2020 Christopher A. Taylor

/*
    Image format conversion
*/

#pragma once

#include "kvm_core.hpp"
#include "kvm_frame.hpp"

namespace kvm {


//------------------------------------------------------------------------------
// Convert

// Convert from JPEG colorspace YUV422 -> YUV420 BT.709
void Convert_YUVJ422_To_BT709_YUV420(const std::shared_ptr<Frame>& frame);


} // namespace kvm
