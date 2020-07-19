// Copyright 2020 Christopher A. Taylor

#include "kvm_capture.hpp"

namespace kvm {


//------------------------------------------------------------------------------
// V4L2

bool V4L2Capture::Initialize()
{
	struct v4l2_capability cap{};
	if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap)) 
	{
        return false;
	}

    return true;
}

void V4L2Capture::Shutdown()
{
    
}


} // namespace kvm
