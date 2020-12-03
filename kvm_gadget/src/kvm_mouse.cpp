// Copyright 2020 Christopher A. Taylor

#include "kvm_mouse.hpp"
#include "kvm_logger.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

namespace kvm {

static logger::Channel Logger("Gadget");


//------------------------------------------------------------------------------
// MouseEmulator

bool MouseEmulator::Initialize()
{
    fd = open("/dev/hidg1", O_RDWR);
    if (fd < 0) {
        Logger.Error("Failed to open mouse emulator device");
        return false;
    }

    Logger.Info("Mouse emulator ready");
    return true;
}

void MouseEmulator::Shutdown()
{
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

bool MouseEmulator::SendReport(uint8_t button_status, int16_t x, int16_t y)
{
    std::lock_guard<std::mutex> locker(Lock);

    char buffer[8] = {0};

    buffer[0] = modifier_keys;

    for (int i = 0; i < keypress_count && i < 6; ++i) {
        buffer[2 + i] = keypresses[i];
    }

    //Logger.Info("SendReport: ", HexDump((const uint8_t*)buffer, 8));

    ssize_t written = write(fd, buffer, 8);
    if (written != 8) {
        Logger.Error("Failed to write mouse device: errno=", errno);
        return false;
    }

    return true;
}


} // namespace kvm
