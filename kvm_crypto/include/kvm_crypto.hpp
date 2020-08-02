// Copyright 2020 Christopher A. Taylor

/*
    Cryptography for Raspberry Pi

    SRTP:
    https://tools.ietf.org/html/rfc3711

    AES in SRTP:
    https://tools.ietf.org/html/rfc6188

    HMAC in SRTP:
    https://tools.ietf.org/html/rfc2104
*/

#include "kvm_core.hpp"
#include "aes128.h"
#include "sha1.h"

namespace kvm {


//------------------------------------------------------------------------------
// Constants

#define KVM_HW_RNG "/dev/hwrng"
#define KVM_MIX_RNG "/dev/urandom"


//------------------------------------------------------------------------------
// Random

// Read random data from RNG
void FillRandom(void* data, int bytes);


//------------------------------------------------------------------------------
// Encrypt

#if 0
class Aes128CtrEncrypt
{
public:
    ~Aes128CtrEncrypt();
    void SetKey(const uint8_t key[AES_128_key_bytes], const uint8_t nonce[AES_128_nonce_bytes]);

    static int DataBytesToBlockBytes(int bytes)
    {
        return (bytes + 15) & ~15;
    }
    void Encrypt(const void* data, uint8_t* output, int bytes);

protected:
    AES_128_ctx State;
};
#endif


//------------------------------------------------------------------------------
// Hash

class Sha1Hash
{
public:
    Sha1Hash();
    ~Sha1Hash();
    void Update(const void* data, int bytes);
    void Final(uint8_t hash[SHA1_DIGEST_SIZE]);

protected:
    sha1_state State;
};


//------------------------------------------------------------------------------
// HMAC-SHA1

void hmac_sha1(
    const void* text, int text_len,
    const void* key, int key_len,
    uint8_t digest[SHA1_DIGEST_SIZE]);


} // namespace kvm
