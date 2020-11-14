// Copyright 2020 Christopher A. Taylor

#include "kvm_capture.hpp"
#include "kvm_logger.hpp"

#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <sstream>

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

bool V4L2Capture::Initialize(FrameHandler handler)
{
    Handler = handler;

    ScopedFunction fail_scope([this]() {
        Shutdown();
    });

    const char* devices[] = {
        "/dev/video0",
        "/dev/video1",
        "/dev/video2",
        "/dev/video3",
        nullptr
    };

    int index;
    for (index = 0; devices[index]; ++index)
    {
        // Open device
        fd = open(devices[index], O_RDWR | O_NONBLOCK);
        if (fd <= 0) {
            Logger.Error("Unable to open ", devices[index], ": ", errno_str());
        }

        // Check if it is a video capture device
        int input = 0;
        if (safe_ioctl(fd, VIDIOC_G_INPUT, &input) == 0) {
            struct v4l2_input vin;

            vin.index = input;
            if (safe_ioctl(fd, VIDIOC_ENUMINPUT, &vin) >= 0) {
                if (vin.status == 0) {
                    Logger.Info("Video ", devices[index], " input ", input, " (", vin.name, ": OK)");
                    break;
                }

                Logger.Error("Video ", devices[index], " input ", input, " (", vin.name, ": err=0x", std::hex, vin.status, ")");
            } else {
                Logger.Error("Video ", devices[index], " input ", input, " VIDIOC_ENUMINPUT failed");
            }
        } else {
            Logger.Error("Video ", devices[index], " VIDIOC_ENUMINPUT failed");
        }

        close(fd);
        fd = -1;
    }
    if (fd < 0) {
        Logger.Error("No capture devices available");
        return false;
    }
    Logger.Info("Opened device: ", devices[index]);

    if (!RequestBuffers(kCameraBufferCount)) {
        return false;
    }

    for (unsigned i = 0; i < kCameraBufferCount; ++i)
    {
        struct v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        int r = safe_ioctl(fd, VIDIOC_QUERYBUF, &buf);
        if (r < 0) {
            Logger.Error("VIDIOC_QUERYBUF i=", i, " failed: ", errno_str());
            return false;
        }

        auto& buffer = Buffers[i];
        buffer.Queued = false;
        buffer.AppOwns = false;
        buffer.Bytes = buf.length;
        buffer.Image = (uint8_t*)mmap(
            nullptr,
            buf.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            buf.m.offset);

        if (!buffer.Image) {
            Logger.Error("mmap i=", i, " failed: ", errno_str());
            return false;
        }

        if (!QueueBuffer(i)) {
            return false;
        }
    }

    if (!Start()) {
        return false;
    }

    Terminated = false;
    ErrorState = false;
    Thread = std::make_shared<std::thread>(&V4L2Capture::Loop, this);

    fail_scope.Cancel();
    return true;
}

int V4L2Capture::GetAppOwnedCount() const
{
    int count = 0;
    for (auto& buffer : Buffers) {
        if (buffer.AppOwns) {
            ++count;
        }
    }
    return count;
}

void V4L2Capture::Shutdown()
{
    if (fd == -1) {
        return;
    }

    Terminated = true;
    JoinThread(Thread);

    Stop();

    for (;;)
    {
        const int count = GetAppOwnedCount();
        if (count > 0) {
            Logger.Warn("Waiting for ", count, " buffers to be returned by application");
            ThreadSleepForMsec(250);
            continue;
        }

        Logger.Info("Application has returned all buffers");
        break;
    }

    Logger.Info("Unmapping buffers");

    for (auto& buffer : Buffers) {
        if (buffer.Image) {
            munmap(buffer.Image, buffer.Bytes);
            buffer.Image = nullptr;
        }
    }

    RequestBuffers(0);

    close(fd);
    fd = -1;
}

void V4L2Capture::Loop()
{
    SetCurrentThreadName("Capture");

    Logger.Info("Capture loop started");

    uint64_t t0 = GetTimeMsec();

    while (!Terminated) {
        bool got_frame = AcquireFrame();

        if (!got_frame) {
            uint64_t t1 = GetTimeMsec();
            int64_t dt = t1 - t0;
            if (dt > 2000) {
                Logger.Error("Camera has not been producing frames");
                t0 = t1;
                ErrorState = true;
            }
            ThreadSleepForMsec(10);
        } else {
            t0 = GetTimeMsec();
        }
    }

    Logger.Info("Capture loop terminated");
}

bool V4L2Capture::RequestBuffers(unsigned count)
{
    Logger.Info("REQBUFS ", count);

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

bool V4L2Capture::QueueBuffer(unsigned index)
{
    auto& buffer = Buffers[index];
    if (buffer.Queued) {
        Logger.Error("FIXME: Double queue");
    }
    buffer.Queued = true;
    buffer.AppOwns = false;

    struct v4l2_buffer buf{};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;

    int r = safe_ioctl(fd, VIDIOC_QBUF, &buf);
    if (r < 0) {
        Logger.Error("VIDIOC_QBUF i=", index, " failed: ", errno_str());
        buffer.Queued = false;
        return false;
    }
    return true;
}

bool V4L2Capture::Start()
{
    if (fd < 0) {
        return false;
    }
    Logger.Info("STREAMON");

    int buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int r = safe_ioctl(fd, VIDIOC_STREAMON, &buf_type);
    if (r < 0) {
        Logger.Error("VIDIOC_STREAMON failed: ", errno_str());
        return false;
    }
    return true;
}

bool V4L2Capture::Stop()
{
    if (fd < 0) {
        return true;
    }
    Logger.Info("STREAMOFF");

    int buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int r = safe_ioctl(fd, VIDIOC_STREAMOFF, &buf_type);
    if (r < 0) {
        Logger.Error("VIDIOC_STREAMOFF failed: ", errno_str());
        return false;
    }
    return true;
}

bool V4L2Capture::AcquireFrame()
{
    struct pollfd desc{};
    desc.fd = fd;
    desc.events = POLLIN;

    // Use a short timeout
    int r = poll(&desc, 1, 100);
    if (r < 0) {
        Logger.Error("poll failed: ", errno_str());
        return false;
    }
    if (r == 0) {
        return false; // Timeout
    }

    struct v4l2_buffer buf{};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    r = safe_ioctl(fd, VIDIOC_DQBUF, &buf);
    if (r < 0) {
        Logger.Error("VIDIOC_DQBUF failed: ", errno_str());

        // Flag error state instantly on device removal error
        if (errno == ENODEV || errno == EINVAL) {
            ErrorState = true;
            Terminated = true;
            ThreadSleepForMsec(100);
        }

        return false;
    }

    const int index = buf.index;
    if (index < 0 || index >= (int)kCameraBufferCount) {
        Logger.Error("buf.index invalid");
        return false;
    }

    if ((buf.flags & V4L2_BUF_FLAG_ERROR) != 0) {
        Logger.Warn("V4L2 reported a recoverable streaming error");
    }

    auto& buffer = Buffers[index];
    buffer.AppOwns = true;
    buffer.Queued = false;

    std::shared_ptr<CameraFrame> frame = std::make_shared<CameraFrame>();
    frame->FrameNumber = buf.sequence;
    frame->ShutterUsec = buf.timestamp.tv_sec * UINT64_C(1000000) + buf.timestamp.tv_usec;
    frame->Image = buffer.Image;
    frame->ImageBytes = buf.bytesused;
    frame->ReleaseFunc = [this, index]() {
        QueueBuffer(index);
    };

    Handler(frame);
    return true;
}


} // namespace kvm
