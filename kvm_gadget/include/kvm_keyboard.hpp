// Copyright 2020 Christopher A. Taylor

/*
    USB Keyboard Emulation

    For keyboard input, it makes the most sense for us to have the Javascript
    code track the currently pressed keys and convert to keyboard scan codes.

    The keyboard modifier + scan codes are very compact, taking just 2 bytes
    for a single key down event.

    References:
    [1] https://wiki.osdev.org/USB_Human_Interface_Devices
    [2] https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf#page=53
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

        Modifier keys bits described in [1].
        Key presses are scan codes from [2].
    */
    bool SendReport(uint8_t modifier_keys, const uint8_t* keypresses, int keypress_count);

protected:
    int fd = -1;
    std::mutex Lock;
};


} // namespace kvm
