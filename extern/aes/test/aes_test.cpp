// Copyright 2020 Christopher A. Taylor

#include "aes256.h"

#include <iostream>
using namespace std;

#include <string.h>

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    cout << "aes_test" << endl;

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

    AES_256_keyschedule(key, p.rk);

    AES_256_encrypt_ctr(&p, in, out, LEN);

    return 0;
}
