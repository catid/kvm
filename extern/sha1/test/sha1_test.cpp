// Copyright 2020 Christopher A. Taylor

#include "sha1.h"

#include <iostream>
#include <vector>
using namespace std;

#include <string.h>

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    cout << "sha1_test" << endl;

    const char* TestVector0 = "abc";
    const uint8_t Hash0[SHA1_DIGEST_SIZE] = {
        0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d
    };

    const char* TestVector1 = "";
    const uint8_t Hash1[SHA1_DIGEST_SIZE] = {
        0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55, 0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09
    };

    const char* TestVector2 = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const uint8_t Hash2[SHA1_DIGEST_SIZE] = {
        0x84, 0x98, 0x3e, 0x44, 0x1c, 0x3b, 0xd2, 0x6e, 0xba, 0xae, 0x4a, 0xa1, 0xf9, 0x51, 0x29, 0xe5, 0xe5, 0x46, 0x70, 0xf1
    };

    const char* TestVector3 = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    const uint8_t Hash3[SHA1_DIGEST_SIZE] = {
        0xa4, 0x9b, 0x24, 0x46, 0xa0, 0x2c, 0x64, 0x5b, 0xf4, 0x19, 0xf9, 0x95, 0xb6, 0x70, 0x91, 0x25, 0x3a, 0x04, 0xa2, 0x59
    };

    const int TestVector4_bytes = 1000000;
    std::vector<char> TestVector4(TestVector4_bytes);
    memset(TestVector4.data(), 0x61, TestVector4_bytes);
    const uint8_t Hash4[SHA1_DIGEST_SIZE] = {
        0x34, 0xaa, 0x97, 0x3c, 0xd4, 0xc4, 0xda, 0xa4, 0xf6, 0x1e, 0xeb, 0x2b, 0xdb, 0xad, 0x27, 0x31, 0x65, 0x34, 0x01, 0x6f
    };

    uint8_t hash[SHA1_DIGEST_SIZE];
    sha1_state state{};

    sha1_neon_init(&state);
    sha1_neon_update(&state, (const uint8_t*)TestVector0, strlen(TestVector0));
    sha1_neon_final(&state, hash);

    if (0 != memcmp(Hash0, hash, SHA1_DIGEST_SIZE)) {
        cout << "TEST FAIL: Test vector 0 does not match" << endl;
        return -1;
    }

    sha1_neon_init(&state);
    sha1_neon_update(&state, (const uint8_t*)TestVector1, strlen(TestVector1));
    sha1_neon_final(&state, hash);

    if (0 != memcmp(Hash1, hash, SHA1_DIGEST_SIZE)) {
        cout << "TEST FAIL: Test vector 1 does not match" << endl;
        return -1;
    }

    sha1_neon_init(&state);
    sha1_neon_update(&state, (const uint8_t*)TestVector2, strlen(TestVector2));
    sha1_neon_final(&state, hash);

    if (0 != memcmp(Hash2, hash, SHA1_DIGEST_SIZE)) {
        cout << "TEST FAIL: Test vector 2 does not match" << endl;
        return -1;
    }

    sha1_neon_init(&state);
    sha1_neon_update(&state, (const uint8_t*)TestVector3, strlen(TestVector3));
    sha1_neon_final(&state, hash);

    if (0 != memcmp(Hash3, hash, SHA1_DIGEST_SIZE)) {
        cout << "TEST FAIL: Test vector 3 does not match" << endl;
        return -1;
    }

    sha1_neon_init(&state);
    sha1_neon_update(&state, (const uint8_t*)TestVector4.data(), TestVector4_bytes);
    sha1_neon_final(&state, hash);

    if (0 != memcmp(Hash4, hash, SHA1_DIGEST_SIZE)) {
        cout << "TEST FAIL: Test vector 4 does not match" << endl;
        return -1;
    }

    cout << "Test success" << endl;
    return 0;
}
