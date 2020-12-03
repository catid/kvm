// Copyright 2020 Christopher A. Taylor

#include "kvm_transport.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

static logger::Channel Logger("MouseTest");

int main(int argc, char* argv[])
{
    SetCurrentThreadName("Main");

    CORE_UNUSED(argc);
    CORE_UNUSED(argv);

    Logger.Info("kvm_mouse_test");

    MouseEmulator mouse;

    if (!mouse.Initialize()) {
        Logger.Error("Mouse initialization failed");
        return kAppFail;
    }

    InputTransport transport;
    transport.Mouse = &mouse;

    // Left Click and release
    uint8_t reports[14] = {
        0x01, 0x85, 0x00, 0x10, 0x00, 0x10, 0x00,
        0x02, 0x85, 0x00, 0x10, 0x00, 0x10, 0x00,
    };

    if (!transport.ParseReports(reports, 14)) {
        Logger.Error("ParseReports failed");
        return kAppFail;
    }

    return kAppSuccess;
}
