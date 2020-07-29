// Copyright 2020 Christopher A. Taylor

/*
    Cryptography for Raspberry Pi
*/

#include "kvm_core.hpp"

namespace kvm {


//------------------------------------------------------------------------------
// Constants

#define KVM_HW_RNG "/dev/hwrng"
#define KVM_MIX_RNG "/dev/urandom"


//------------------------------------------------------------------------------
// Random

// Read random data from RNG
void FillRandom(void* data, int bytes);


} // namespace kvm
