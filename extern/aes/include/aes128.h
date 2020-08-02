/*
    References:
    [1] AES implementation from
    https://github.com/Ko-/aes-armcortexm

    Note: https://github.com/Ko-/aes-armcortexm/issues/1
*/

#pragma once

#include <stdint.h>

extern "C" {


#define AES_128_nonce_bytes 16 /* 128-bit */
#define AES_128_key_bytes 16 /* 128-bit */

typedef struct AES_128_ctx_t {
    // Low 2-4 bytes are used for nonce, remaining bytes are for IV
    uint8_t nonce_iv[AES_128_nonce_bytes];
    uint8_t key[AES_128_key_bytes];
    uint8_t rk[10*16];
} AES_128_ctx;

extern void AES_128_keyschedule(const uint8_t* in_key, uint8_t* rk);

extern void AES_128_encrypt_ctr(AES_128_ctx const* ctx, const uint8_t* pt, uint8_t* ct, uint32_t bytes);


} // extern "C"
