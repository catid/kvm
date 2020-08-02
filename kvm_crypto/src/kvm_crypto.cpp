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

void AesCtrEncrypt::Encrypt(const void* data, uint8_t* output, int bytes)
{
    AES_256_encrypt_ctr(&State, (const uint8_t*)data, output, bytes);
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

void Sha1Hash::Update(const void* data, int bytes)
{
    sha1_neon_update(&State, (const uint8_t*)data, (unsigned)bytes);
}

void Sha1Hash::Final(uint8_t hash[SHA1_DIGEST_SIZE])
{
    sha1_neon_final(&State, hash);
}


//------------------------------------------------------------------------------
// HMAC-SHA1

void hmac_sha1(
    const void* text, int text_len,
    const void* key, int key_len,
    uint8_t digest[SHA1_DIGEST_SIZE])
{
    // Inner padding: key XOR ipad
    uint8_t k_ipad[65] = { 0 };

    // Outer padding: key XOR opad
    uint8_t k_opad[65] = { 0 };

    uint8_t tk[SHA1_DIGEST_SIZE];
    int i;

    // if key is longer than 64 bytes reset it to key=MD5(key)
    if (key_len > 64) {
        Sha1Hash tctx;
        tctx.Update(key, key_len);
        tctx.Final(tk);

        key = tk;
        key_len = SHA1_DIGEST_SIZE;
    }

    /*
     * the HMAC_MD5 transform looks like:
     *
     * MD5(K XOR opad, MD5(K XOR ipad, text))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated 64 times
     * opad is the byte 0x5c repeated 64 times
     * and text is the data being protected
     */

    // start out by storing key in pads
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    // XOR key with ipad and opad values
    for (i=0; i<64; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    // Inner hash
    Sha1Hash inner;
    inner.Update(k_ipad, 64);
    inner.Update(text, text_len);
    inner.Final(digest);

    // Outer hash
    Sha1Hash outer;
    outer.Update(k_opad, 64);
    outer.Update(digest, SHA1_DIGEST_SIZE);
    outer.Final(digest);
}


} // namespace kvm
