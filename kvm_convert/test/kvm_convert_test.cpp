// Copyright 2020 Christopher A. Taylor

#include "kvm_turbojpeg.hpp"
#include "kvm_capture.hpp"
#include "kvm_convert.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

static logger::Channel Logger("TurboJpegTest");

#include <csignal>
#include <atomic>
std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
void SignalHandler(int)
{
    Terminated = true;
}

int main(int argc, char* argv[])
{
    SetCurrentThreadName("Main");

    CORE_UNUSED(argc);
    CORE_UNUSED(argv);

    Logger.Info("kvm_turbojpeg_test");

    V4L2Capture capture;

    TurboJpegDecoder decoder;

    if (!capture.Initialize([&](const std::shared_ptr<CameraFrame>& buffer) {
        Logger.Info("Got frame #", buffer->FrameNumber, " bytes = ", buffer->ImageBytes);

        uint64_t t0 = GetTimeUsec();

        auto frame = decoder.Decompress(buffer->Image, buffer->ImageBytes);
        if (!frame) {
            Logger.Error("Failed to decode JPEG");
            return;
        }

        uint64_t t1 = GetTimeUsec();

        Convert_YUVJ422_To_BT709_YUV420(frame);

        uint64_t t2 = GetTimeUsec();

        Logger.Info("Decoding JPEG took ", (t1 - t0) / 1000.f, " msec");
        Logger.Info("Convert from YUVJ422 to YUV420 BT.709 took ", (t2 - t1) / 1000.f, " msec");

        decoder.Release(frame);
    })) {
        Logger.Error("Failed to start capture");
        return kAppFail;
    }

    std::signal(SIGINT, SignalHandler);

    while (!Terminated && !capture.IsError()) {
        ThreadSleepForMsec(100);
    }

    Logger.Info("Shutting down...");

    capture.Shutdown();

    return kAppSuccess;
}
