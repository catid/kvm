// Copyright 2020 Christopher A. Taylor

#include "kvm_gadget.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

static logger::Channel Logger("GadgetTest");

int main(int argc, char* argv[])
{
    SetCurrentThreadName("Main");

    CORE_UNUSED(argc);
    CORE_UNUSED(argv);

    Logger.Info("kvm_gadget_test");

    KeyboardEmulator keyboard;

    if (!keyboard.Initialize()) {
        Logger.Error("Keyboard initialization failed");
        return kAppFail;
    }

    keyboard.Send('A');

    MouseEmulator mouse;

    if (!mouse.Initialize()) {
        Logger.Error("Mouse initialization failed");
        return kAppFail;
    }

    mouse.MouseMove(0, 0);

    mouse.MouseLeftClick(true);

    return kAppSuccess;
}
