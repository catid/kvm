// Copyright 2020 Christopher A. Taylor

#include "kvm_capture.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

static logger::Channel Logger("CaptureTest");

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

    Logger.Info("kvm_capture_test");

    V4L2Capture capture;

    if (!capture.Initialize([](const std::shared_ptr<CameraFrame>& buffer) {
        Logger.Info("Got frame #", buffer->FrameNumber, " bytes = ", buffer->ImageBytes);
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
