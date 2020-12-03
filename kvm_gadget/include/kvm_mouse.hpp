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

        Button status bits described in [1].

        x,y range from 0..32767 where 0,0 = upper left and 32767,32767 is the
        lower right of the screen.  This is covered somewhat by [2].
    */
    bool SendReport(uint8_t button_status, uint16_t x, uint16_t y);

protected:
    int fd = -1;
    std::mutex Lock;
};


} // namespace kvm
