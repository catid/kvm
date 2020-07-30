/*
    References:
    [1] SHA1 implementation from
    https://github.com/ivanich/android_kernel_htc_pyramid/commit/28c6eecbb14e0532fba721f922732bae2dbcd1be#diff-3f6ab5f2fab5abad02550e72dcac95e9
*/

#pragma once

#include <stdint.h>

extern "C" {

#define SHA1_DIGEST_SIZE        20
#define SHA1_BLOCK_SIZE         64

#define SHA1_H0		0x67452301UL
#define SHA1_H1		0xefcdab89UL
#define SHA1_H2		0x98badcfeUL
#define SHA1_H3		0x10325476UL
#define SHA1_H4		0xc3d2e1f0UL


struct sha1_state {
	uint64_t count;
	uint32_t state[SHA1_DIGEST_SIZE / 4];
	uint8_t buffer[SHA1_BLOCK_SIZE];
};

void sha1_neon_init(struct sha1_state *sctx);

int sha1_neon_update(struct sha1_state *sctx, const uint8_t *data, unsigned int len);

int sha1_neon_final(struct sha1_state *sctx, uint8_t *out);


} // extern "C"
