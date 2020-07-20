// Copyright 2020 Christopher A. Taylor

/*
    Core tools used by multiple KVM sub-projects
*/

#include "kvm_core.hpp"

#include <linux/videodev2.h>

namespace kvm {


//------------------------------------------------------------------------------
// Constants

#define KVM_VIDEO_DEVICE "/dev/video0"

static const unsigned kCameraBufferCount = 8;


//------------------------------------------------------------------------------
// Tools

// Convert errno to string
std::string errno_str();

// Retry ioctls until they succeed
int safe_ioctl(int fd, unsigned request, void* arg);


//------------------------------------------------------------------------------
// V4L2

class V4L2Capture
{
public:
    ~V4L2Capture()
    {
        Shutdown();
    }

    bool Initialize();
    void Shutdown();

protected:
    int fd = -1;

    bool RequestBuffers(unsigned count);
};



} // namespace kvm
