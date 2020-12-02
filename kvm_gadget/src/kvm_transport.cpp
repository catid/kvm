// Copyright 2020 Christopher A. Taylor

#include "kvm_transport.hpp"
#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Transport");


//------------------------------------------------------------------------------
// Tools

void Invert_convertUint8ArrayToBinaryString(
    const char* buf,
    int len,
    std::vector<uint8_t>& data)
{
    // Output data is no larger than len
    data.resize(len);

    int output_count = 0;

    for (int i = 0; i < len - 1; ++i) {
        const uint8_t x = buf[i];

        // If this is one of the bytes that has the high bits:
        if (i % 8 == 7) {
            for (int j = 0; j < 7; ++j) {
                data[output_count - 7 + j] |= (x << (7 - j)) & 0x80;
            }
        } else {
            data[output_count++] = x;
        }
    }

    // If it is not an even multiple of 8:
    if (len % 8 != 0) {
        // Handle last few missing bits
        const uint8_t x = buf[len - 1];

        const int partial_count = (len % 8) - 1;

        for (int j = 0; j < partial_count; ++j) {
            data[output_count - partial_count + j] |= (x << (7 - j)) & 0x80;
        }
    }

    data.resize(output_count);
}


//------------------------------------------------------------------------------
// InputTransport

bool InputTransport::ParseReports(const uint8_t* data, int bytes)
{
    bool success = true;

    while (bytes >= 3)
    {
        Counter32 id = Counter32::ExpandFromTruncatedWithBias(PrevIdentifier, Counter8(data[0]), -32);

        const int bits = data[1];
        const int count = bits & 0x7f;

        // If ID is new:
        if (id > PrevIdentifier) {
            // Store ID
            PrevIdentifier = id;

            const bool is_mouse = (bits & 0x80) != 0;

            // If this is a mouse:
            if (is_mouse) {
                // FIXME
            } else { // keyboard:
                // Send contained report
                success &= Keyboard->SendReport(data[2], data + 3, count - 1);
            }
        }

        data += 2 + count;
        bytes -= 2 + count;
    }

    return success;
}


} // namespace kvm
