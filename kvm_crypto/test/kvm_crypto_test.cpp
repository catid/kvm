// Copyright 2020 Christopher A. Taylor

#include "kvm_crypto.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

static logger::Channel Logger("CryptoTest");

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

bool TestSHA1Hmac()
{
    uint8_t digest[SHA1_DIGEST_SIZE];

    // Test vectors from:
    // https://tools.ietf.org/html/rfc2202

    const int key_1_len = 20;
    const uint8_t key_1[key_1_len] = {
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
    };
    const char* text_1 = "Hi There";
    const int text_1_len = (int)strlen(text_1);
    hmac_sha1(text_1, text_1_len, key_1, key_1_len, digest);
    const uint8_t digest_1[SHA1_DIGEST_SIZE] = {
        0xb6, 0x17, 0x31, 0x86, 0x55, 0x05, 0x72, 0x64, 0xe2, 0x8b,
        0xc0, 0xb6, 0xfb, 0x37, 0x8c, 0x8e, 0xf1, 0x46, 0xbe, 0x00
    };
    if (0 != memcmp(digest_1, digest, SHA1_DIGEST_SIZE)) {
        Logger.Error("HMAC-SHA1 test vector 1 mismatch");
        return false;
    }

    const char* key_2 = "Jefe";
    const int key_2_len = (int)strlen(key_2);
    const char* text_2 = "what do ya want for nothing?";
    const int text_2_len = (int)strlen(text_2);
    hmac_sha1(text_2, text_2_len, key_2, key_2_len, digest);
    const uint8_t digest_2[SHA1_DIGEST_SIZE] = {
        0xef, 0xfc, 0xdf, 0x6a, 0xe5, 0xeb, 0x2f, 0xa2, 0xd2, 0x74,
        0x16, 0xd5, 0xf1, 0x84, 0xdf, 0x9c, 0x25, 0x9a, 0x7c, 0x79
    };
    if (0 != memcmp(digest_2, digest, SHA1_DIGEST_SIZE)) {
        Logger.Error("HMAC-SHA1 test vector 2 mismatch");
        return false;
    }

    const int key_3_len = 20;
    uint8_t key_3[key_3_len];
    memset(key_3, 0xaa, key_3_len);
    const int text_3_len = 50;
    uint8_t text_3[text_3_len];
    memset(text_3, 0xdd, text_3_len);
    hmac_sha1(text_3, text_3_len, key_3, key_3_len, digest);
    const uint8_t digest_3[SHA1_DIGEST_SIZE] = {
        0x12, 0x5d, 0x73, 0x42, 0xb9, 0xac, 0x11, 0xcd, 0x91, 0xa3,
        0x9a, 0xf4, 0x8a, 0xa1, 0x7b, 0x4f, 0x63, 0xf1, 0x75, 0xd3
    };
    if (0 != memcmp(digest_3, digest, SHA1_DIGEST_SIZE)) {
        Logger.Error("HMAC-SHA1 test vector 3 mismatch");
        return false;
    }

    // Skip

    const int key_7_len = 80;
    uint8_t key_7[key_7_len];
    memset(key_7, 0xaa, key_7_len);
    const char* text_7 = "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data";
    const int text_7_len = (int)strlen(text_7);
    hmac_sha1(text_7, text_7_len, key_7, key_7_len, digest);
    const uint8_t digest_7[SHA1_DIGEST_SIZE] = {
        0xe8, 0xe9, 0x9d, 0x0f, 0x45, 0x23, 0x7d, 0x78, 0x6d, 0x6b,
        0xba, 0xa7, 0x96, 0x5c, 0x78, 0x08, 0xbb, 0xff, 0x1a, 0x91
    };
    if (0 != memcmp(digest_7, digest, SHA1_DIGEST_SIZE)) {
        Logger.Error("HMAC-SHA1 test vector 7 mismatch");
        return false;
    }

    return true;
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

    TestSHA1();
    if (!TestSHA1Hmac()) {
        Logger.Error("ERROR: HMAC-SHA1 test failed");
        return kAppFail;
    } else {
        Logger.Info("HMAC-SHA1 test success");
    }

    return kAppSuccess;
}
