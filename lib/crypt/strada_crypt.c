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
 * strada_crypt.c - Password hashing wrapper for Strada FFI
 *
 * This library wraps the C crypt.h functions for secure password hashing.
 * It accepts StradaValue* arguments directly from Strada's FFI.
 *
 * Return types:
 *   - Functions returning int return int (wrapped by dl_call_int_sv)
 *   - Functions returning string return char* (wrapped by dl_call_str_sv)
 *
 * Compile with:
 *   gcc -shared -fPIC -o libstrada_crypt.so strada_crypt.c -lcrypt -I../../runtime
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <crypt.h>

/* Include Strada runtime for StradaValue type */
#include "strada_runtime.h"

/* Last error message */
static char last_error[256] = "";

/* Base64-like alphabet for salt generation */
static const char salt_chars[] =
    "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

/* Generate random bytes from /dev/urandom */
static int get_random_bytes(unsigned char *buf, size_t len) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        snprintf(last_error, sizeof(last_error), "Failed to open /dev/urandom");
        return -1;
    }

    ssize_t n = read(fd, buf, len);
    close(fd);

    if (n != (ssize_t)len) {
        snprintf(last_error, sizeof(last_error), "Failed to read random bytes");
        return -1;
    }

    return 0;
}

/* Generate random salt characters */
static void generate_salt_chars(char *buf, size_t len) {
    unsigned char random[64];
    if (get_random_bytes(random, len) < 0) {
        /* Fallback to less secure random */
        srand(time(NULL) ^ getpid());
        for (size_t i = 0; i < len; i++) {
            random[i] = rand() & 0xFF;
        }
    }

    for (size_t i = 0; i < len; i++) {
        buf[i] = salt_chars[random[i] % 64];
    }
    buf[len] = '\0';
}

/* Internal: generate salt for algorithm (returns static buffer) */
static const char* gen_salt_internal(const char *algorithm, int rounds) {
    static char salt[128];
    char random_chars[32];

    last_error[0] = '\0';

    if (!algorithm) {
        algorithm = "sha512";
    }

    if (strcmp(algorithm, "des") == 0) {
        /* DES: 2 character salt */
        generate_salt_chars(random_chars, 2);
        snprintf(salt, sizeof(salt), "%s", random_chars);
    }
    else if (strcmp(algorithm, "md5") == 0) {
        /* MD5: $1$salt$ */
        generate_salt_chars(random_chars, 8);
        snprintf(salt, sizeof(salt), "$1$%s$", random_chars);
    }
    else if (strcmp(algorithm, "sha256") == 0) {
        /* SHA-256: $5$salt$ or $5$rounds=N$salt$ */
        generate_salt_chars(random_chars, 16);
        if (rounds > 0) {
            if (rounds < 1000) rounds = 1000;
            if (rounds > 999999999) rounds = 999999999;
            snprintf(salt, sizeof(salt), "$5$rounds=%d$%s$", rounds, random_chars);
        } else {
            snprintf(salt, sizeof(salt), "$5$%s$", random_chars);
        }
    }
    else if (strcmp(algorithm, "sha512") == 0) {
        /* SHA-512: $6$salt$ or $6$rounds=N$salt$ */
        generate_salt_chars(random_chars, 16);
        if (rounds > 0) {
            if (rounds < 1000) rounds = 1000;
            if (rounds > 999999999) rounds = 999999999;
            snprintf(salt, sizeof(salt), "$6$rounds=%d$%s$", rounds, random_chars);
        } else {
            snprintf(salt, sizeof(salt), "$6$%s$", random_chars);
        }
    }
    else if (strcmp(algorithm, "bcrypt") == 0) {
        /* bcrypt: $2b$cost$22charsalt */
        generate_salt_chars(random_chars, 22);
        int cost = (rounds > 0) ? rounds : 12;
        if (cost < 4) cost = 4;
        if (cost > 31) cost = 31;
        snprintf(salt, sizeof(salt), "$2b$%02d$%s", cost, random_chars);
    }
    else {
        snprintf(last_error, sizeof(last_error), "Unknown algorithm: %s", algorithm);
        salt[0] = '\0';
    }

    return salt;
}

/* Hash a password with the given salt
 * Returns the hashed password as a newly allocated string (caller must free) */
char* strada_crypt_hash(StradaValue *password_sv, StradaValue *salt_sv) {
    const char *password = strada_to_str(password_sv);
    const char *salt = strada_to_str(salt_sv);

    last_error[0] = '\0';

    if (!password || !salt) {
        snprintf(last_error, sizeof(last_error), "Invalid password or salt");
        return strdup("");
    }

    /* Use crypt_r for thread safety */
    struct crypt_data data;
    memset(&data, 0, sizeof(data));

    char *result = crypt_r(password, salt, &data);

    if (!result) {
        snprintf(last_error, sizeof(last_error), "crypt() failed");
        return strdup("");
    }

    /* Check for error indicators */
    if (result[0] == '*') {
        snprintf(last_error, sizeof(last_error), "Invalid salt or algorithm not supported");
        return strdup("");
    }

    return strdup(result);
}

/* Generate a salt for the specified algorithm
 * Returns newly allocated string (caller must free) */
char* strada_crypt_gen_salt(StradaValue *algorithm_sv) {
    const char *algorithm = strada_to_str(algorithm_sv);
    return strdup(gen_salt_internal(algorithm, 0));
}

/* Generate a salt with custom rounds
 * Returns newly allocated string (caller must free) */
char* strada_crypt_gen_salt_rounds(StradaValue *algorithm_sv, StradaValue *rounds_sv) {
    const char *algorithm = strada_to_str(algorithm_sv);
    int rounds = (int)strada_to_int(rounds_sv);
    return strdup(gen_salt_internal(algorithm, rounds));
}

/* Verify a password against a hash
 * Returns 1 if match, 0 otherwise */
int strada_crypt_verify(StradaValue *password_sv, StradaValue *hash_sv) {
    const char *password = strada_to_str(password_sv);
    const char *hash = strada_to_str(hash_sv);

    last_error[0] = '\0';

    if (!password || !hash || strlen(hash) == 0) {
        return 0;
    }

    /* Hash the password with the stored hash as salt */
    struct crypt_data data;
    memset(&data, 0, sizeof(data));

    char *result = crypt_r(password, hash, &data);

    if (!result) {
        return 0;
    }

    /* Constant-time comparison to prevent timing attacks */
    size_t hash_len = strlen(hash);
    size_t result_len = strlen(result);

    if (hash_len != result_len) {
        return 0;
    }

    int diff = 0;
    for (size_t i = 0; i < hash_len; i++) {
        diff |= hash[i] ^ result[i];
    }

    return (diff == 0) ? 1 : 0;
}

/* Get the algorithm used in a hash string
 * Returns newly allocated string (caller must free) */
char* strada_crypt_get_algorithm(StradaValue *hash_sv) {
    const char *hash = strada_to_str(hash_sv);

    if (!hash || strlen(hash) < 3) {
        return strdup("unknown");
    }

    if (hash[0] != '$') {
        /* DES - no prefix */
        return strdup("des");
    }

    if (strncmp(hash, "$1$", 3) == 0) {
        return strdup("md5");
    }
    else if (strncmp(hash, "$5$", 3) == 0) {
        return strdup("sha256");
    }
    else if (strncmp(hash, "$6$", 3) == 0) {
        return strdup("sha512");
    }
    else if (strncmp(hash, "$2a$", 4) == 0 ||
             strncmp(hash, "$2b$", 4) == 0 ||
             strncmp(hash, "$2y$", 4) == 0) {
        return strdup("bcrypt");
    }
    else if (strncmp(hash, "$y$", 3) == 0) {
        return strdup("yescrypt");
    }

    return strdup("unknown");
}

/* Check if an algorithm is supported
 * Returns 1 if supported, 0 otherwise */
int strada_crypt_is_supported(StradaValue *algorithm_sv) {
    const char *algorithm = strada_to_str(algorithm_sv);

    if (!algorithm) {
        return 0;
    }

    /* Generate a test salt and try to hash */
    const char *salt = gen_salt_internal(algorithm, 0);

    if (strlen(salt) == 0) {
        return 0;
    }

    struct crypt_data data;
    memset(&data, 0, sizeof(data));

    char *result = crypt_r("test", salt, &data);

    if (!result || result[0] == '*') {
        return 0;
    }

    return 1;
}

/* Get last error message
 * Returns pointer to static buffer (do not free) */
char* strada_crypt_error(void) {
    return strdup(last_error);
}
