// Copyright 2020 Christopher A. Taylor

#include "kvm_capture.hpp"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <sstream>

#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Capture");


//------------------------------------------------------------------------------
// Tools

std::string errno_str()
{
    std::ostringstream oss;
    oss << errno << ": " << strerror(errno);
    return oss.str();
}

int safe_ioctl(int fd, unsigned request, void* arg)
{
    int r;
    do {
        r = ioctl(fd, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}


//------------------------------------------------------------------------------
// V4L2

bool V4L2Capture::Initialize()
{
    ScopedFunction fail_scope([this]() {
        Shutdown();
    });

    fd = open(KVM_VIDEO_DEVICE, O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        Logger.Error("Unable to open: ", KVM_VIDEO_DEVICE);
        return false;
    }

    if (!RequestBuffers(kCameraBufferCount)) {
        return false;
    }

    fail_scope.Cancel();
    return true;
}

void V4L2Capture::Shutdown()
{
    RequestBuffers(0);

    if (fd != -1) {
        close(fd);
        fd = -1;
    }
}

bool V4L2Capture::RequestBuffers(unsigned count)
{
    struct v4l2_requestbuffers rb{};
    rb.count = count;
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_MMAP;

    int r = safe_ioctl(fd, VIDIOC_REQBUFS, &rb);
    if (r < 0 || rb.count != count) {
        Logger.Error("VIDIOC_REQBUFS failed: ", errno_str(), " rb.count=", rb.count);
        return false;
    }

    return true;
}


} // namespace kvm
