// Copyright 2020 Christopher A. Taylor

#include "kvm_crypto.hpp"
#include "kvm_logger.hpp"

#include "blake3.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

namespace kvm {

static logger::Channel Logger("Crypto");


//------------------------------------------------------------------------------
// Tools

// Prevent compiler from optimizing out memset()
static void secure_zero_memory(void* v, size_t n)
{
    volatile uint8_t* p = (volatile uint8_t*)v;

    for (size_t i = 0; i < n; ++i) {
        p[i] = 0;
    }
}

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
    blake3_hasher hasher;
    uint8_t key[64] = {};

    // Wipe stack memory before leaving
    ScopedFunction clean_scope([&]() {
        secure_zero_memory(key, sizeof(key));
        secure_zero_memory(&hasher, sizeof(hasher));
    });

    // Fill as many random bytes as we can get
    read_file(KVM_HW_RNG, key, 32);
    read_file(KVM_MIX_RNG, key + 32, 32);

    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, key, 64);
    blake3_hasher_finalize(&hasher, (uint8_t*)data, bytes);
}


//------------------------------------------------------------------------------
// Encrypt

AesCtrEncrypt::~AesCtrEncrypt()
{
    secure_zero_memory(&State, sizeof(State));
}

void AesCtrEncrypt::SetKey(const uint8_t key[AES_256_key_bytes], const uint8_t nonce[AES_256_nonce_bytes])
{
    State.ctr = 0;
    memcpy(State.nonce, nonce, AES_256_nonce_bytes);
    memcpy(State.key, key, AES_256_key_bytes);

    AES_256_keyschedule(key, State.rk);
}

void AesCtrEncrypt::Encrypt(const uint8_t* data, uint8_t* output, int bytes)
{
    AES_256_encrypt_ctr(&State, data, output, bytes);
}


//------------------------------------------------------------------------------
// Hash

Sha1Hash::Sha1Hash()
{
    sha1_neon_init(&State);
}

Sha1Hash::~Sha1Hash()
{
    secure_zero_memory(&State, sizeof(State));
}

void Sha1Hash::Update(const uint8_t* data, int bytes)
{
    sha1_neon_update(&State, data, (unsigned)bytes);
}

void Sha1Hash::Final(uint8_t hash[SHA1_DIGEST_SIZE])
{
    sha1_neon_final(&State, hash);
}


} // namespace kvm
