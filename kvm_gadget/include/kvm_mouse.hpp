// Copyright 2020 Christopher A. Taylor

/*
    USB Mouse Emulation

    Mouse motion described by the USB HID reports is all relative.

    References:
    [1] https://wiki.osdev.org/USB_Human_Interface_Devices
    [2] http://embeddedguruji.blogspot.com/2019/04/learning-usb-hid-in-linux-part-6.html
*/

#pragma once

#include "kvm_core.hpp"

#include "Counter.h"

namespace kvm {


//------------------------------------------------------------------------------
// MouseEmulator

class MouseEmulator
{
public:
    ~MouseEmulator()
    {
        Shutdown();
    }

    bool Initialize();
    void Shutdown();

    /*
        Send a USB HID mouse report.

        This is thread-safe.
    */
    bool SendReport(uint8_t button_status, int16_t x, int16_t y);

protected:
    int fd = -1;
    std::mutex Lock;
};


} // namespace kvm
