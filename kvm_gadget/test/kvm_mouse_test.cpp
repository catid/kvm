// Copyright 2020 Christopher A. Taylor

#include "kvm_transport.hpp"
#include "kvm_logger.hpp"
#include "kvm_serializer.hpp"
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

    srand(GetTimeUsec());

    uint8_t reports[7] = {
        0x01, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    // Move cursor to a random position
    WriteU16_LE(reports + 3, rand() % 32768);
    WriteU16_LE(reports + 5, rand() % 32768);

    if (!transport.ParseReports(reports, 7)) {
        Logger.Error("ParseReports failed");
        return kAppFail;
    }

    return kAppSuccess;
}
