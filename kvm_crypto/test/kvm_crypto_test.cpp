// Copyright 2020 Christopher A. Taylor

#include "kvm_crypto.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

#include "aes256.h"

static logger::Channel Logger("CryptoTest");

void TestAES()
{
    const uint32_t LEN = 256*16;
    const uint32_t LEN_ROUNDED = ((LEN+15)/16)*16;

    const uint8_t nonce[12] = {1,2,3,1,2,4,1,2,5,1,2,6};
    const uint8_t key[32] = {4,5,6,7,4,5,6,8,4,5,6,9,4,5,6,10,4,5,6,11,4,5,6,12,4,5,6,13,4,5,6,14};
    uint8_t in[LEN];
    uint8_t out[LEN_ROUNDED];

    unsigned int i;
    for(i=0;i<LEN;++i) {
        in[i] = i%256;
    }

    AES_256_param p;
    p.ctr = 0;
    memcpy(p.nonce, nonce, 12);
    memcpy(p.key, key, 32);

    for (int i = 0; i < 10; ++i) {
        uint64_t t0 = GetTimeUsec();
        AES_256_keyschedule(key, p.rk);
        uint64_t t1 = GetTimeUsec();
        Logger.Info("AES_256_keyschedule in ", (t1 - t0) / 1000.f, " msec");
    }

    for (int i = 0; i < 10; ++i) {
        uint64_t t0 = GetTimeUsec();
        AES_256_encrypt_ctr(&p, in, out, LEN);
        uint64_t t1 = GetTimeUsec();
        Logger.Info("AES_256_encrypt_ctr in ", (t1 - t0) / 1000.f / (LEN_ROUNDED / 16), " msec per block");
    }
}

void TestSHA1()
{
    for (int i = 0; i < 10; ++i) {
        uint64_t t0 = GetTimeUsec();

        kvm::Sha1Hash hash;

        uint8_t data[1200] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        hash.Update(data, 1200);

        std::vector<uint8_t> digest(SHA1_DIGEST_SIZE);
        hash.Final(digest.data());

        uint64_t t1 = GetTimeUsec();
        Logger.Info("sha1 in ", (t1 - t0) / 1000.f, " msec for 1200 bytes");
    }
}

int main(int argc, char* argv[])
{
    SetCurrentThreadName("Main");

    CORE_UNUSED(argc);
    CORE_UNUSED(argv);

    Logger.Info("kvm_crypto_test");

    uint8_t key[32];

    for (int i = 0; i < 10; ++i) {
        uint64_t t0 = GetTimeUsec();
        FillRandom(key, 32);
        uint64_t t1 = GetTimeUsec();
        Logger.Info("Generated key: ", BinToBase64(key, 32), " in ", (t1 - t0) / 1000.f, " msec");
    }

    TestAES();
    TestSHA1();

    return kAppSuccess;
}
