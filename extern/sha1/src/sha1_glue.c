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

extern void sha1_transform_neon(void *state_h, const char *data,
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

    return __sha1_neon_update(desc, data, len, partial);
}

/* Add padding and return the message digest. */
int sha1_neon_final(struct sha1_state *sctx, uint8_t *out)
{
    unsigned int i, index, padlen;
    __be32 *dst = (__be32 *)out;
    __be64 bits;
    static const uint8_t padding[SHA1_BLOCK_SIZE] = { 0x80, };

    bits = cpu_to_be64(sctx->count << 3);

    /* Pad out to 56 mod 64 and append length */
    index = sctx->count % SHA1_BLOCK_SIZE;
    padlen = (index < 56) ? (56 - index) : ((SHA1_BLOCK_SIZE+56) - index);

    /* We need to fill a whole block for __sha1_neon_update() */
    if (padlen <= 56) {
        sctx->count += padlen;
        memcpy(sctx->buffer + index, padding, padlen);
    } else {
        __sha1_neon_update(desc, padding, padlen, index);
    }
    __sha1_neon_update(desc, (const uint8_t *)&bits, sizeof(bits), 56);

    /* Store state in digest */
    for (i = 0; i < 5; i++) {
        dst[i] = cpu_to_be32(sctx->state[i]);
    }

    /* Wipe context */
    memset(sctx, 0, sizeof(*sctx));

    return 0;
}
