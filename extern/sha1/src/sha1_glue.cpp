/*
 * Glue code for the SHA1 Secure Hash Algorithm assembler implementation using
 * ARM NEON instructions.
 *
 * Copyright © 2014 Jussi Kivilinna <jussi.kivilinna@iki.fi>
 *
 * This file is based on sha1_generic.c and sha1_ssse3_glue.c:
 *  Copyright (c) Alan Smithee.
 *  Copyright (c) Andrew McDonald <andrew@mcdonald.org.uk>
 *  Copyright (c) Jean-Francois Dive <jef@linuxbe.org>
 *  Copyright (c) Mathias Krause <minipli@googlemail.com>
 *  Copyright (c) Chandramouli Narayanan <mouli@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#include "sha1.h"

#include <string.h>

extern "C" void sha1_transform_neon(void *state_h, const uint8_t *data,
                    unsigned int rounds);

void sha1_neon_init(struct sha1_state *sctx)
{
    sctx->count = 0;
    sctx->state[0] = SHA1_H0;
    sctx->state[1] = SHA1_H1;
    sctx->state[2] = SHA1_H2;
    sctx->state[3] = SHA1_H3;
    sctx->state[4] = SHA1_H4;
}

static int __sha1_neon_update(struct sha1_state *sctx, const uint8_t *data,
                   unsigned int len, unsigned int partial)
{
    unsigned int done = 0;

    sctx->count += len;

    if (partial) {
        done = SHA1_BLOCK_SIZE - partial;
        memcpy(sctx->buffer + partial, data, done);
        sha1_transform_neon(sctx->state, sctx->buffer, 1);
    }

    if (len - done >= SHA1_BLOCK_SIZE) {
        const unsigned int rounds = (len - done) / SHA1_BLOCK_SIZE;

        sha1_transform_neon(sctx->state, data + done, rounds);
        done += rounds * SHA1_BLOCK_SIZE;
    }

    memcpy(sctx->buffer, data + done, len - done);

    return 0;
}

int sha1_neon_update(struct sha1_state *sctx, const uint8_t *data, unsigned int len)
{
    unsigned int partial = sctx->count % SHA1_BLOCK_SIZE;

    /* Handle the fast case right here */
    if (partial + len < SHA1_BLOCK_SIZE) {
        sctx->count += len;
        memcpy(sctx->buffer + partial, data, len);

        return 0;
    }

    return __sha1_neon_update(sctx, data, len, partial);
}

// Swaps byte order in a 16-bit word
inline uint16_t ByteSwap16(uint16_t word)
{
    return (word >> 8) | (word << 8);
}

// Swaps byte order in a 32-bit word
inline uint32_t ByteSwap32(uint32_t word)
{
    const uint16_t swapped_old_hi = ByteSwap16(static_cast<uint16_t>(word >> 16));
    const uint16_t swapped_old_lo = ByteSwap16(static_cast<uint16_t>(word));
    return (static_cast<uint32_t>(swapped_old_lo) << 16) | swapped_old_hi;
}

// Swaps byte order in a 64-bit word
inline uint64_t ByteSwap64(uint64_t word)
{
    const uint32_t swapped_old_hi = ByteSwap32(static_cast<uint32_t>(word >> 32));
    const uint32_t swapped_old_lo = ByteSwap32(static_cast<uint32_t>(word));
    return (static_cast<uint64_t>(swapped_old_lo) << 32) | swapped_old_hi;
}


/* Add padding and return the message digest. */
int sha1_neon_final(struct sha1_state *sctx, uint8_t *out)
{
    unsigned int i, index, padlen;
    uint32_t *dst = (uint32_t *)out;
    uint64_t bits;
    static const uint8_t padding[SHA1_BLOCK_SIZE] = { 0x80, };

    bits = ByteSwap64(sctx->count << 3);

    /* Pad out to 56 mod 64 and append length */
    index = sctx->count % SHA1_BLOCK_SIZE;
    padlen = (index < 56) ? (56 - index) : ((SHA1_BLOCK_SIZE + 56) - index);

    /* We need to fill a whole block for __sha1_neon_update() */
    if (padlen <= 56) {
        sctx->count += padlen;
        memcpy(sctx->buffer + index, padding, padlen);
    } else {
        __sha1_neon_update(sctx, padding, padlen, index);
    }
    __sha1_neon_update(sctx, (const uint8_t *)&bits, (unsigned)sizeof(bits), 56);

    /* Store state in digest */
    for (i = 0; i < 5; i++) {
        dst[i] = ByteSwap32(sctx->state[i]);
    }

    /* Wipe context */
    memset(sctx, 0, sizeof(*sctx));

    return 0;
}
