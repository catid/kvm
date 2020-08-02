// Copyright 2020 Christopher A. Taylor

#include "aes256.h"

#include <iostream>
using namespace std;

#include <string.h>
#include <vector>

static uint8_t hex_to_byte(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'z') {
        return ch - 'a' + 10;
    }
    return 0xff;
}

static uint8_t bytestr_to_byte(const char* bytestr)
{
    return (hex_to_byte(bytestr[0]) << 4) | hex_to_byte(bytestr[1]);
}

static void str_to_buf(std::vector<uint8_t>& buf, const char* s)
{
    const int len = (int)strlen(s);
    buf.resize(len/2);

    for (int i = 0; i < len/2; ++i) {
        buf[i] = bytestr_to_byte(s + len-2 + i*2);
    }
}

static bool run_aes(const uint8_t* key, const uint8_t* ctr, const uint8_t* pt, const uint8_t* ct, int bytes)
{
    AES_256_param p;
    memcpy(p.nonce_iv, ctr, AES_256_nonce_bytes);
    memcpy(p.key, key, AES_256_key_bytes);
    AES_256_keyschedule(p.key, p.rk);

    std::vector<uint8_t> output(bytes + 16);
    AES_256_encrypt_ctr(&p, pt, output.data(), bytes);

    if (0 != memcmp(output.data(), ct, bytes)) {
        cout << "ERROR: Ciphertext mismatch" << endl;
        return false;
    }
    return true;
}

bool TestAES()
{
    // Test vectors from:
    // https://tools.ietf.org/html/rfc3686#page-9

    std::vector<uint8_t> key, ctr, pt, ct;
    str_to_buf(key, "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    str_to_buf(ctr, "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    str_to_buf(pt, "6bc1bee22e409f96e93d7e117393172a");
    str_to_buf(ct, "601ec313775789a5b7a7f504bbf3d228");

    if (!run_aes(key.data(), ctr.data(), pt.data(), ct.data(), pt.size())) {
        cout << "ERROR: Test vector 1 failed" << endl;
        return false;
    }

    return true;
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    cout << "aes_test" << endl;

    if (!TestAES()) {
        cout << "ERROR: Test failed" << endl;
        return -1;
    }
    cout << "Test success" << endl;

    return 0;
}
