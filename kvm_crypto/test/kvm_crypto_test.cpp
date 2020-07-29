// Copyright 2020 Christopher A. Taylor

#include "kvm_crypto.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

static logger::Channel Logger("CryptoTest");

int main(int argc, char* argv[])
{
    SetCurrentThreadName("Main");

    CORE_UNUSED(argc);
    CORE_UNUSED(argv);

    Logger.Info("kvm_crypto_test");

    uint8_t key[32];

    FillRandom(key, 32);

    Logger.Info("Generated key: ", BinToBase64(key, 32));

    return kAppSuccess;
}
