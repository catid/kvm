// Copyright 2020 Christopher A. Taylor

#include "kvm_crypto.hpp"
#include "kvm_logger.hpp"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

namespace kvm {

static logger::Channel Logger("Crypto");


//------------------------------------------------------------------------------
// Tools

static int read_file(const char* path, uint8_t* buffer, int bytes)
{
    int completed = 0, retries = 100;

    do {
        int len = bytes - completed;

        // Attempt to open the file
        int fd = open(path, O_RDONLY);
        if (fd >= 0) {
            // Set non-blocking mode
            int flags = fcntl(fd, F_GETFL, 0);
            fcntl(fd, F_SETFL, flags | O_NONBLOCK);

            // Launch read request and close file
            len = read(fd, buffer, len);
            close(fd);
        }

        // If read request failed (ie. blocking):
        if (len <= 0) {
            if (--retries >= 0) {
                continue;
            } else {
                // Give up
                break;
            }
        }

        // Subtract off the read byte count from request size
        completed += len;
        buffer += len;
    } while (completed < bytes);

    // Return number of bytes completed
    return completed;
}


//------------------------------------------------------------------------------
// Random

void FillRandom(void* data, int bytes)
{
    int written = read_file(KVM_HW_RNG, (uint8_t*)data, bytes);
    if (written < bytes) {
        Logger.Warn("Only read ", written, " of ", bytes, " bytes from ", KVM_HW_RNG);
        bytes -= written;
        written = read_file(KVM_FALLBACK_RNG, (uint8_t*)data + written, bytes);
        if (written < bytes) {
            Logger.Error("Only read ", written, " of ", bytes, " bytes from ", KVM_FALLBACK_RNG);
        }
    }
}


} // namespace kvm
