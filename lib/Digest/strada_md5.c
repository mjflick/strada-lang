/*
 This file is part of the Strada Language (https://github.com/mjflick/strada-lang).
 Copyright (c) 2026 Michael J. Flickinger

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, version 2.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * strada_md5.c - MD5 Message-Digest Algorithm for Strada FFI
 *
 * Implementation based on RFC 1321.
 *
 * Compile with:
 *   gcc -shared -fPIC -o libstrada_md5.so strada_md5.c -I../../runtime
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Include Strada runtime for StradaValue type */
#include "strada_runtime.h"

/* MD5 context structure */
typedef struct {
    uint32_t state[4];      /* state (ABCD) */
    uint32_t count[2];      /* number of bits, modulo 2^64 */
    uint8_t buffer[64];     /* input buffer */
} MD5_CTX;

/* Constants for MD5Transform routine */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

/* F, G, H and I are basic MD5 functions */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
#define FF(a, b, c, d, x, s, ac) { \
    (a) += F((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define GG(a, b, c, d, x, s, ac) { \
    (a) += G((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define HH(a, b, c, d, x, s, ac) { \
    (a) += H((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define II(a, b, c, d, x, s, ac) { \
    (a) += I((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}

/* Padding for final block */
static uint8_t PADDING[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Encode input (uint32_t) into output (uint8_t) - little endian */
static void Encode(uint8_t *output, uint32_t *input, unsigned int len) {
    unsigned int i, j;
    for (i = 0, j = 0; j < len; i++, j += 4) {
        output[j] = (uint8_t)(input[i] & 0xff);
        output[j+1] = (uint8_t)((input[i] >> 8) & 0xff);
        output[j+2] = (uint8_t)((input[i] >> 16) & 0xff);
        output[j+3] = (uint8_t)((input[i] >> 24) & 0xff);
    }
}

/* Decode input (uint8_t) into output (uint32_t) - little endian */
static void Decode(uint32_t *output, uint8_t *input, unsigned int len) {
    unsigned int i, j;
    for (i = 0, j = 0; j < len; i++, j += 4) {
        output[i] = ((uint32_t)input[j]) | (((uint32_t)input[j+1]) << 8) |
                    (((uint32_t)input[j+2]) << 16) | (((uint32_t)input[j+3]) << 24);
    }
}

/* MD5 basic transformation - transforms state based on block */
static void MD5Transform(uint32_t state[4], uint8_t block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];

    Decode(x, block, 64);

    /* Round 1 */
    FF(a, b, c, d, x[ 0], S11, 0xd76aa478);
    FF(d, a, b, c, x[ 1], S12, 0xe8c7b756);
    FF(c, d, a, b, x[ 2], S13, 0x242070db);
    FF(b, c, d, a, x[ 3], S14, 0xc1bdceee);
    FF(a, b, c, d, x[ 4], S11, 0xf57c0faf);
    FF(d, a, b, c, x[ 5], S12, 0x4787c62a);
    FF(c, d, a, b, x[ 6], S13, 0xa8304613);
    FF(b, c, d, a, x[ 7], S14, 0xfd469501);
    FF(a, b, c, d, x[ 8], S11, 0x698098d8);
    FF(d, a, b, c, x[ 9], S12, 0x8b44f7af);
    FF(c, d, a, b, x[10], S13, 0xffff5bb1);
    FF(b, c, d, a, x[11], S14, 0x895cd7be);
    FF(a, b, c, d, x[12], S11, 0x6b901122);
    FF(d, a, b, c, x[13], S12, 0xfd987193);
    FF(c, d, a, b, x[14], S13, 0xa679438e);
    FF(b, c, d, a, x[15], S14, 0x49b40821);

    /* Round 2 */
    GG(a, b, c, d, x[ 1], S21, 0xf61e2562);
    GG(d, a, b, c, x[ 6], S22, 0xc040b340);
    GG(c, d, a, b, x[11], S23, 0x265e5a51);
    GG(b, c, d, a, x[ 0], S24, 0xe9b6c7aa);
    GG(a, b, c, d, x[ 5], S21, 0xd62f105d);
    GG(d, a, b, c, x[10], S22, 0x02441453);
    GG(c, d, a, b, x[15], S23, 0xd8a1e681);
    GG(b, c, d, a, x[ 4], S24, 0xe7d3fbc8);
    GG(a, b, c, d, x[ 9], S21, 0x21e1cde6);
    GG(d, a, b, c, x[14], S22, 0xc33707d6);
    GG(c, d, a, b, x[ 3], S23, 0xf4d50d87);
    GG(b, c, d, a, x[ 8], S24, 0x455a14ed);
    GG(a, b, c, d, x[13], S21, 0xa9e3e905);
    GG(d, a, b, c, x[ 2], S22, 0xfcefa3f8);
    GG(c, d, a, b, x[ 7], S23, 0x676f02d9);
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);

    /* Round 3 */
    HH(a, b, c, d, x[ 5], S31, 0xfffa3942);
    HH(d, a, b, c, x[ 8], S32, 0x8771f681);
    HH(c, d, a, b, x[11], S33, 0x6d9d6122);
    HH(b, c, d, a, x[14], S34, 0xfde5380c);
    HH(a, b, c, d, x[ 1], S31, 0xa4beea44);
    HH(d, a, b, c, x[ 4], S32, 0x4bdecfa9);
    HH(c, d, a, b, x[ 7], S33, 0xf6bb4b60);
    HH(b, c, d, a, x[10], S34, 0xbebfbc70);
    HH(a, b, c, d, x[13], S31, 0x289b7ec6);
    HH(d, a, b, c, x[ 0], S32, 0xeaa127fa);
    HH(c, d, a, b, x[ 3], S33, 0xd4ef3085);
    HH(b, c, d, a, x[ 6], S34, 0x04881d05);
    HH(a, b, c, d, x[ 9], S31, 0xd9d4d039);
    HH(d, a, b, c, x[12], S32, 0xe6db99e5);
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8);
    HH(b, c, d, a, x[ 2], S34, 0xc4ac5665);

    /* Round 4 */
    II(a, b, c, d, x[ 0], S41, 0xf4292244);
    II(d, a, b, c, x[ 7], S42, 0x432aff97);
    II(c, d, a, b, x[14], S43, 0xab9423a7);
    II(b, c, d, a, x[ 5], S44, 0xfc93a039);
    II(a, b, c, d, x[12], S41, 0x655b59c3);
    II(d, a, b, c, x[ 3], S42, 0x8f0ccc92);
    II(c, d, a, b, x[10], S43, 0xffeff47d);
    II(b, c, d, a, x[ 1], S44, 0x85845dd1);
    II(a, b, c, d, x[ 8], S41, 0x6fa87e4f);
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0);
    II(c, d, a, b, x[ 6], S43, 0xa3014314);
    II(b, c, d, a, x[13], S44, 0x4e0811a1);
    II(a, b, c, d, x[ 4], S41, 0xf7537e82);
    II(d, a, b, c, x[11], S42, 0xbd3af235);
    II(c, d, a, b, x[ 2], S43, 0x2ad7d2bb);
    II(b, c, d, a, x[ 9], S44, 0xeb86d391);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    /* Zeroize sensitive information */
    memset(x, 0, sizeof(x));
}

/* MD5 initialization - begins an MD5 operation */
static void MD5Init(MD5_CTX *context) {
    context->count[0] = context->count[1] = 0;
    /* Load magic initialization constants */
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}

/* MD5 block update operation - continues an MD5 message-digest operation */
static void MD5Update(MD5_CTX *context, uint8_t *input, unsigned int inputLen) {
    unsigned int i, index, partLen;

    /* Compute number of bytes mod 64 */
    index = (unsigned int)((context->count[0] >> 3) & 0x3F);

    /* Update number of bits */
    if ((context->count[0] += ((uint32_t)inputLen << 3)) < ((uint32_t)inputLen << 3)) {
        context->count[1]++;
    }
    context->count[1] += ((uint32_t)inputLen >> 29);

    partLen = 64 - index;

    /* Transform as many times as possible */
    if (inputLen >= partLen) {
        memcpy(&context->buffer[index], input, partLen);
        MD5Transform(context->state, context->buffer);

        for (i = partLen; i + 63 < inputLen; i += 64) {
            MD5Transform(context->state, &input[i]);
        }

        index = 0;
    } else {
        i = 0;
    }

    /* Buffer remaining input */
    memcpy(&context->buffer[index], &input[i], inputLen - i);
}

/* MD5 finalization - ends an MD5 message-digest operation */
static void MD5Final(uint8_t digest[16], MD5_CTX *context) {
    uint8_t bits[8];
    unsigned int index, padLen;

    /* Save number of bits */
    Encode(bits, context->count, 8);

    /* Pad out to 56 mod 64 */
    index = (unsigned int)((context->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    MD5Update(context, PADDING, padLen);

    /* Append length (before padding) */
    MD5Update(context, bits, 8);

    /* Store state in digest */
    Encode(digest, context->state, 16);

    /* Zeroize sensitive information */
    memset(context, 0, sizeof(*context));
}

/* Compute MD5 of a buffer - returns 16-byte digest */
static void md5_compute(const uint8_t *data, size_t len, uint8_t digest[16]) {
    MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, (uint8_t *)data, (unsigned int)len);
    MD5Final(digest, &ctx);
}

/* Base64 encoding table */
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Encode 16 bytes to base64 (24 chars + null) */
static void base64_encode_16(const uint8_t *input, char *output) {
    int i, j;
    for (i = 0, j = 0; i < 15; i += 3, j += 4) {
        output[j]   = b64_table[input[i] >> 2];
        output[j+1] = b64_table[((input[i] & 0x03) << 4) | (input[i+1] >> 4)];
        output[j+2] = b64_table[((input[i+1] & 0x0f) << 2) | (input[i+2] >> 6)];
        output[j+3] = b64_table[input[i+2] & 0x3f];
    }
    /* Last byte (16th) - 1 byte remaining = 2 base64 chars + 2 padding */
    output[20] = b64_table[input[15] >> 2];
    output[21] = b64_table[(input[15] & 0x03) << 4];
    /* Note: Perl's Digest::MD5 omits padding for md5_base64 */
    output[22] = '\0';
}

/* ========== Strada FFI Functions ========== */

/*
 * md5($data) - Returns raw 16-byte MD5 digest
 * Returns: 16-byte binary string
 */
StradaValue* strada_digest_md5(StradaValue *data_sv) {
    const char *data;
    size_t len;
    uint8_t digest[16];

    /* Get string data with length (binary-safe) */
    if (data_sv->type == STRADA_STR && data_sv->value.pv) {
        data = data_sv->value.pv;
        len = strada_str_len(data_sv);
    } else {
        data = strada_to_str(data_sv);
        len = strlen(data);
    }

    md5_compute((const uint8_t *)data, len, digest);

    /* Return as binary string */
    return strada_new_str_len((char *)digest, 16);
}

/*
 * md5_hex($data) - Returns MD5 digest as 32-char hex string
 * Returns: 32-character lowercase hex string
 */
char* strada_digest_md5_hex(StradaValue *data_sv) {
    const char *data;
    size_t len;
    uint8_t digest[16];
    char *hex;
    int i;

    /* Get string data with length (binary-safe) */
    if (data_sv->type == STRADA_STR && data_sv->value.pv) {
        data = data_sv->value.pv;
        len = strada_str_len(data_sv);
    } else {
        data = strada_to_str(data_sv);
        len = strlen(data);
    }

    md5_compute((const uint8_t *)data, len, digest);

    /* Convert to hex string */
    hex = malloc(33);
    if (!hex) return strdup("");

    for (i = 0; i < 16; i++) {
        sprintf(&hex[i*2], "%02x", digest[i]);
    }
    hex[32] = '\0';

    return hex;
}

/*
 * md5_base64($data) - Returns MD5 digest as base64 string (22 chars, no padding)
 * Returns: 22-character base64 string (Perl-compatible, no = padding)
 */
char* strada_digest_md5_base64(StradaValue *data_sv) {
    const char *data;
    size_t len;
    uint8_t digest[16];
    char *b64;

    /* Get string data with length (binary-safe) */
    if (data_sv->type == STRADA_STR && data_sv->value.pv) {
        data = data_sv->value.pv;
        len = strada_str_len(data_sv);
    } else {
        data = strada_to_str(data_sv);
        len = strlen(data);
    }

    md5_compute((const uint8_t *)data, len, digest);

    /* Convert to base64 (22 chars for 16 bytes, no padding) */
    b64 = malloc(23);
    if (!b64) return strdup("");

    base64_encode_16(digest, b64);

    return b64;
}
