/*
 * This file is part of the Strada Language (https://github.com/mjflick/strada-lang).
 * Copyright (c) 2026 Michael J. Flickinger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * strada_compress.c - Compression library for Strada using zlib
 *
 * Provides gzip and deflate compression via zlib.
 * Build: gcc -shared -fPIC -o libstrada_compress.so strada_compress.c -lz
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "../../runtime/strada_runtime.h"

/* Compress data using gzip format */
StradaValue* strada_gzip_compress(StradaValue* data) {
    const char* input = strada_to_str(data);
    size_t input_len = strlen(input);

    if (input_len == 0) {
        return strada_new_str("");
    }

    /* Allocate output buffer (worst case: input + gzip overhead) */
    size_t output_size = compressBound(input_len) + 18;
    char* output = malloc(output_size);
    if (!output) {
        return strada_new_str("");
    }

    /* Initialize zlib stream for gzip (windowBits = 15 + 16 for gzip) */
    z_stream strm;
    memset(&strm, 0, sizeof(strm));

    int ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                           15 + 16, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        free(output);
        return strada_new_str("");
    }

    strm.next_in = (Bytef*)input;
    strm.avail_in = input_len;
    strm.next_out = (Bytef*)output;
    strm.avail_out = output_size;

    ret = deflate(&strm, Z_FINISH);
    deflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        free(output);
        return strada_new_str("");
    }

    /* Create Strada string with binary data */
    StradaValue* result = strada_new_str_len(output, strm.total_out);
    free(output);
    return result;
}

/* Compress data using deflate format (no header) */
StradaValue* strada_deflate_compress(StradaValue* data) {
    const char* input = strada_to_str(data);
    size_t input_len = strlen(input);

    if (input_len == 0) {
        return strada_new_str("");
    }

    size_t output_size = compressBound(input_len);
    char* output = malloc(output_size);
    if (!output) {
        return strada_new_str("");
    }

    /* Initialize zlib stream for raw deflate (windowBits = -15) */
    z_stream strm;
    memset(&strm, 0, sizeof(strm));

    int ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                           -15, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        free(output);
        return strada_new_str("");
    }

    strm.next_in = (Bytef*)input;
    strm.avail_in = input_len;
    strm.next_out = (Bytef*)output;
    strm.avail_out = output_size;

    ret = deflate(&strm, Z_FINISH);
    deflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        free(output);
        return strada_new_str("");
    }

    StradaValue* result = strada_new_str_len(output, strm.total_out);
    free(output);
    return result;
}

/* Decompress gzip data */
StradaValue* strada_gzip_decompress(StradaValue* data) {
    const char* input = strada_to_str(data);
    size_t input_len = strada_str_len(data);

    if (input_len == 0) {
        return strada_new_str("");
    }

    /* Start with 4x input size, grow if needed */
    size_t output_size = input_len * 4;
    if (output_size < 1024) output_size = 1024;
    char* output = malloc(output_size);
    if (!output) {
        return strada_new_str("");
    }

    /* Initialize zlib for gzip decompression (windowBits = 15 + 16) */
    z_stream strm;
    memset(&strm, 0, sizeof(strm));

    int ret = inflateInit2(&strm, 15 + 16);
    if (ret != Z_OK) {
        free(output);
        return strada_new_str("");
    }

    strm.next_in = (Bytef*)input;
    strm.avail_in = input_len;
    strm.next_out = (Bytef*)output;
    strm.avail_out = output_size;

    /* Decompress in a loop in case we need to grow buffer */
    size_t total_out = 0;
    while (1) {
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_END) {
            total_out = strm.total_out;
            break;
        }
        if (ret != Z_OK && ret != Z_BUF_ERROR) {
            inflateEnd(&strm);
            free(output);
            return strada_new_str("");
        }
        if (strm.avail_out == 0) {
            /* Need more output space */
            size_t new_size = output_size * 2;
            char* new_output = realloc(output, new_size);
            if (!new_output) {
                inflateEnd(&strm);
                free(output);
                return strada_new_str("");
            }
            output = new_output;
            strm.next_out = (Bytef*)(output + output_size);
            strm.avail_out = new_size - output_size;
            output_size = new_size;
        }
    }

    inflateEnd(&strm);

    StradaValue* result = strada_new_str_len(output, total_out);
    free(output);
    return result;
}

/* Check if gzip compression is worthwhile (returns 1 if yes) */
StradaValue* strada_should_compress(StradaValue* content_type, StradaValue* data) {
    const char* ct = strada_to_str(content_type);
    size_t len = strlen(strada_to_str(data));

    /* Don't compress if too small (< 1KB) */
    if (len < 1024) {
        return strada_new_int(0);
    }

    /* Compress text-based content types */
    if (strstr(ct, "text/") ||
        strstr(ct, "application/json") ||
        strstr(ct, "application/javascript") ||
        strstr(ct, "application/xml") ||
        strstr(ct, "application/xhtml") ||
        strstr(ct, "+xml") ||
        strstr(ct, "+json")) {
        return strada_new_int(1);
    }

    /* Don't compress already-compressed formats */
    if (strstr(ct, "image/") ||
        strstr(ct, "video/") ||
        strstr(ct, "audio/") ||
        strstr(ct, "application/zip") ||
        strstr(ct, "application/gzip") ||
        strstr(ct, "application/x-gzip")) {
        return strada_new_int(0);
    }

    return strada_new_int(0);
}
