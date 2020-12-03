// Copyright 2020 Christopher A. Taylor

/*
    USB Mouse Emulation

    

    References:
    [1] https://wiki.osdev.org/USB_Human_Interface_Devices
*/

#pragma once

#include "kvm_core.hpp"

#include "Counter.h"

namespace kvm {


//------------------------------------------------------------------------------
// KeyboardEmulator

class KeyboardEmulator
{
public:
    ~KeyboardEmulator()
    {
        Shutdown();
    }

    bool Initialize();
    void Shutdown();

    /*
        Send a USB HID keyboard report.

        This is thread-safe.
    */
    bool SendReport(uint8_t modifier_keys, const uint8_t* keypresses, int keypress_count);

protected:
    int fd = -1;
    std::mutex Lock;
};


} // namespace kvm
