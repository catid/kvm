// Copyright 2020 Christopher A. Taylor

#include "kvm_keyboard.hpp"
#include "kvm_logger.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

namespace kvm {

static logger::Channel Logger("Gadget");


//------------------------------------------------------------------------------
// KeyboardEmulator

bool KeyboardEmulator::Initialize()
{
    fd = open("/dev/hidg0", O_RDWR);
    if (fd < 0) {
        Logger.Error("Failed to open keyboard emulator device");
        return false;
    }

    Logger.Info("Keyboard emulator ready");
    return true;
}

void KeyboardEmulator::Shutdown()
{
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

bool KeyboardEmulator::ParseReports(const uint8_t* data, int bytes)
{
    bool success = true;

    while (bytes >= 3)
    {
        Counter32 id = Counter32::ExpandFromTruncatedWithBias(PrevIdentifier, Counter8(data[0]), -32);

        const uint8_t modifier_keys = data[1];
        const int keypress_count = data[2];

        if (id > PrevIdentifier) {
            PrevIdentifier = id;
            success &= SendReport(modifier_keys, data + 3, keypress_count);
        }

        data += 3 + keypress_count;
        bytes -= 3 + keypress_count;
    }

    return success;
}

bool KeyboardEmulator::SendReport(uint8_t modifier_keys, const uint8_t* keypresses, int keypress_count)
{
    char buffer[8] = {0};

    buffer[0] = modifier_keys;

    for (int i = 0; i < keypress_count && i < 6; ++i) {
        buffer[2 + i] = keypresses[i];
    }

    Logger.Info("SendReport: ", HexDump((const uint8_t*)buffer, 8));

    ssize_t written = write(fd, buffer, 8);
    if (written != 8) {
        Logger.Error("Failed to write keyboard device: errno=", errno);
        return false;
    }

    return true;
}


} // namespace kvm
