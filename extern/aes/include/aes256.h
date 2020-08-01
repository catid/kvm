/*
    References:
    [1] AES implementation from
    https://github.com/Ko-/aes-armcortexm
*/

#pragma once

#include <stdint.h>

extern "C" {


#define AES_256_nonce_bytes 12 /* 96-bit */
#define AES_256_key_bytes 32 /* 256-bit */

typedef struct AES_256_param_t {
    uint32_t ctr;
    uint8_t nonce[AES_256_nonce_bytes];
    uint8_t key[AES_256_key_bytes];
    uint8_t rk[13*16];
} AES_256_param;

/*
    Provide 256-bit (32 bytes) input key.
    Pass AES_256_param::rk into rk parameter.
*/
extern void AES_256_keyschedule(const uint8_t* in_key, uint8_t* rk);

/*
    Encrypt input data, expanding it to be a multiple of 16 bytes.
*/
extern void AES_256_encrypt_ctr(AES_256_param const* param, const uint8_t* in_data, uint8_t* out_blocks, uint32_t bytes);


} // extern "C"
