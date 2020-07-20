// Copyright 2020 Christopher A. Taylor

/*
    Core tools used by multiple KVM sub-projects
*/

#include "kvm_core.hpp"

#include <linux/videodev2.h>

#include <memory>
#include <thread>
#include <atomic>

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
// Buffer

struct CameraBuffer
{
    uint8_t* Image = nullptr;
    unsigned Bytes = 0;

    std::atomic<bool> Queued = ATOMIC_VAR_INIT(false);
    std::atomic<bool> AppOwns = ATOMIC_VAR_INIT(false);
};

struct CameraFrame
{
    // Returns frame to V4L2Capture object when it goes out of scope
    ~CameraFrame()
    {
        ReleaseFunc();
    }

    // Metadata
    uint64_t FrameNumber = 0;
    uint64_t ShutterUsec = 0;

    // Pointer to image data
    uint8_t* Image = nullptr;
    unsigned ImageBytes = 0;

    std::function<void()> ReleaseFunc;
};


//------------------------------------------------------------------------------
// V4L2

using FrameHandler = std::function<void(const std::shared_ptr<CameraFrame>& buffer)>;

class V4L2Capture
{
public:
    ~V4L2Capture()
    {
        Shutdown();
    }

    bool Initialize(FrameHandler handler);
    void Shutdown();

    bool Start();
    bool Stop();

    bool IsError() const
    {
        return ErrorState;
    }

protected:
    FrameHandler Handler;

    int fd = -1;
    std::array<CameraBuffer, kCameraBufferCount> Buffers;

    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
    std::atomic<bool> ErrorState = ATOMIC_VAR_INIT(false);
    std::shared_ptr<std::thread> Thread;

    void Loop();

    bool RequestBuffers(unsigned count);
    bool QueueBuffer(unsigned index);
    bool AcquireFrame();
    int GetAppOwnedCount() const;
};



} // namespace kvm
