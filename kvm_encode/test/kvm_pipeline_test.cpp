// Copyright 2020 Christopher A. Taylor

/*
    Test the video pipleine:
    Camera -> JPEG decode -> Encode -> Video Parse
*/

#include "kvm_pipeline.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

static logger::Channel Logger("kvm_pipeline");

#include <exception>
#include <fstream>

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

    Logger.Info("kvm_pipeline_test");

    VideoPipeline pipeline;

    std::ofstream file("output.h264");
    if (!file) {
        Logger.Error("Failed to open output file: output.h264");
        return kAppFail;
    }

    pipeline.Initialize([&](
        uint64_t frame_number,
        uint64_t shutter_usec,
        const uint8_t* data,
        int bytes,
        bool keyframe
    ) {
        Logger.Info("Writing ", keyframe ? "key" : "", "frame ", frame_number, " time=", shutter_usec, " bytes=", bytes);
        file.write((const char*)data, bytes);
    });
    ScopedFunction pipeline_scope([&]() {
        pipeline.Shutdown();
    });

    std::signal(SIGINT, SignalHandler);

    while (!Terminated && !pipeline.IsTerminated()) {
        ThreadSleepForMsec(100);
    }

    Logger.Info("Shutting down...");
    return kAppSuccess;
}
