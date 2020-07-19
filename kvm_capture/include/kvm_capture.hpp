// Copyright 2020 Christopher A. Taylor

/*
    Core tools used by multiple KVM sub-projects
*/

#include "core.hpp"

namespace kvm {


//------------------------------------------------------------------------------
// Constants

#define KVM_VIDEO_DEVICE "/dev/video0"


//------------------------------------------------------------------------------
// V4L2

class V4L2Capture
{
public:
    bool Initialize();
    void Shutdown();

protected:
};



} // namespace kvm
