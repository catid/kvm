// Copyright 2020 Christopher A. Taylor

#include "aes128.h"

#include <iostream>
using namespace std;

#include <string.h>
#include <vector>

static uint8_t hex_to_nibble(char ch)
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
    return (hex_to_nibble(bytestr[0]) << 4) | hex_to_nibble(bytestr[1]);
}

static void str_to_buf(std::vector<uint8_t>& buf, const char* s)
{
    const int len = (int)strlen(s);
    buf.resize(len/2);

    for (int i = 0; i < len/2; ++i) {
        buf[i] = bytestr_to_byte(s + len-2 - i*2);
    }
}

static char nibble_to_hex(uint8_t nibble)
{
    if (nibble >= 10) {
        return 'A' + nibble - 10;
    }
    return '0' + nibble;
}

static void byte_to_str(uint8_t byte, char* s)
{
    s[0] = nibble_to_hex(byte >> 4);
    s[1] = nibble_to_hex(byte & 15);
}

static std::string buf_to_str(const void* data, size_t bytes)
{
    std::string s;
    s.resize(bytes * 2 + 1);

    const uint8_t* buf = (const uint8_t*)data;
    for (int i = 0; i < (int)bytes; ++i) {
        byte_to_str(buf[i], &s.front() + bytes * 2 - 2 - i*2);
    }
    s[bytes*2] = '\0';

    return s;
}

static bool run_aes(const uint8_t* key, const uint8_t* ctr, const uint8_t* pt, const uint8_t* ct, int bytes)
{
    AES_128_ctx p;
    memcpy(p.nonce_iv, ctr, AES_128_nonce_bytes);
    memcpy(p.key, key, AES_128_key_bytes);
    AES_128_keyschedule(key, p.rk);

    std::vector<uint8_t> output(bytes + 16);
    AES_128_encrypt_ctr(&p, pt, output.data(), bytes);

    if (0 != memcmp(output.data(), ct, bytes)) {
        cout << "ERROR: Ciphertext mismatch" << endl;
        cout << "Actual ciphertext: " << buf_to_str(output.data(), bytes) << endl;
        return false;
    }
    return true;
}


bool TestAES()
{
    // Test vectors from:
    // https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf

    std::vector<uint8_t> key, ctr, pt, ct;
    str_to_buf(key, "2b7e151628aed2a6abf7158809cf4f3c");
    str_to_buf(ctr, "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"); // original
    //str_to_buf(ctr, "f0f1f2f3f4f5f6f7f8f9fafbfffefdfc"); // flip lower word bytes
    //str_to_buf(ctr, "fffefdfcf0f1f2f3f4f5f6f7f8f9fafb"); // move lower word to upper word and flip bytes
    //str_to_buf(ctr, "fcfdfefff0f1f2f3f4f5f6f7f8f9fafb"); // move lower word to upper word
    //str_to_buf(ctr, "f4f5f6f7f8f9fafbfcfdfefff0f1f2f3"); // move high 4 bytes to low 4 bytes
    //str_to_buf(ctr, "f4f5f6f7f8f9fafbfcfdfefff3f2f1f0"); // move high 4 bytes to low 4 bytes, flipped
    str_to_buf(pt, "6bc1bee22e409f96e93d7e117393172a");
    str_to_buf(ct, "874d6191b620e3261bef6864990db6ce");

    if (!run_aes(key.data(), ctr.data(), pt.data(), ct.data(), pt.size())) {
        cout << "ERROR: Test vector 1 failed" << endl;
        cout << "key: " << buf_to_str(key.data(), key.size()) << endl;
        cout << "ctr: " << buf_to_str(ctr.data(), ctr.size()) << endl;
        cout << "pt: " << buf_to_str(pt.data(), pt.size()) << endl;
        cout << "ct: " << buf_to_str(ct.data(), ct.size()) << endl;
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
