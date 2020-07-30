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
#include "aes256.h"
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
// Hash

void HmacSha1();


} // namespace kvm
