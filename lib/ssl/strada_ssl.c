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
 * strada_ssl.c - OpenSSL wrapper for Strada FFI
 *
 * This library accepts StradaValue* arguments directly from Strada's FFI
 * and handles type conversion internally.
 *
 * Compile with:
 *   gcc -shared -fPIC -o libstrada_ssl.so strada_ssl.c -lssl -lcrypto -I../../runtime
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/* Include Strada runtime for StradaValue type */
#include "strada_runtime.h"

/* Connection structure holding both socket and SSL context */
typedef struct {
    int socket_fd;
    SSL *ssl;
    SSL_CTX *ctx;
    int is_server;
} SSLConnection;

/* Global initialization flag */
static int ssl_initialized = 0;

/* Initialize OpenSSL library */
int strada_ssl_init(void) {
    if (!ssl_initialized) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        ssl_initialized = 1;
    }
    return 0;
}

/* Cleanup OpenSSL */
void strada_ssl_cleanup(void) {
    if (ssl_initialized) {
        EVP_cleanup();
        ERR_free_strings();
        ssl_initialized = 0;
    }
}

/* Get last error as string */
const char* strada_ssl_error(void) {
    static char errbuf[256];
    unsigned long err = ERR_get_error();
    if (err == 0) {
        return "No error";
    }
    ERR_error_string_n(err, errbuf, sizeof(errbuf));
    return errbuf;
}

/* Create SSL client connection to host:port
 * Accepts StradaValue* arguments from Strada FFI */
SSLConnection* strada_ssl_connect(StradaValue *host_sv, StradaValue *port_sv) {
    if (!ssl_initialized) {
        strada_ssl_init();
    }

    /* Extract actual values from StradaValue */
    const char *host = strada_to_str(host_sv);
    int port = (int)strada_to_int(port_sv);

    SSLConnection *conn = malloc(sizeof(SSLConnection));
    if (!conn) return NULL;

    memset(conn, 0, sizeof(SSLConnection));
    conn->is_server = 0;

    /* Create SSL context */
    const SSL_METHOD *method = TLS_client_method();
    conn->ctx = SSL_CTX_new(method);
    if (!conn->ctx) {
        free(conn);
        return NULL;
    }

    /* Create socket */
    conn->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->socket_fd < 0) {
        SSL_CTX_free(conn->ctx);
        free(conn);
        return NULL;
    }

    /* Resolve hostname */
    struct hostent *he = gethostbyname(host);
    if (!he) {
        close(conn->socket_fd);
        SSL_CTX_free(conn->ctx);
        free(conn);
        return NULL;
    }

    /* Connect to server */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(conn->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(conn->socket_fd);
        SSL_CTX_free(conn->ctx);
        free(conn);
        return NULL;
    }

    /* Create SSL connection */
    conn->ssl = SSL_new(conn->ctx);
    if (!conn->ssl) {
        close(conn->socket_fd);
        SSL_CTX_free(conn->ctx);
        free(conn);
        return NULL;
    }

    SSL_set_fd(conn->ssl, conn->socket_fd);

    /* Set SNI hostname */
    SSL_set_tlsext_host_name(conn->ssl, host);

    /* Perform SSL handshake */
    if (SSL_connect(conn->ssl) <= 0) {
        SSL_free(conn->ssl);
        close(conn->socket_fd);
        SSL_CTX_free(conn->ctx);
        free(conn);
        return NULL;
    }

    return conn;
}

/* Create SSL server socket
 * Accepts StradaValue* arguments from Strada FFI */
SSLConnection* strada_ssl_server(StradaValue *port_sv, StradaValue *cert_file_sv, StradaValue *key_file_sv) {
    if (!ssl_initialized) {
        strada_ssl_init();
    }

    /* Extract actual values from StradaValue */
    int port = (int)strada_to_int(port_sv);
    const char *cert_file = strada_to_str(cert_file_sv);
    const char *key_file = strada_to_str(key_file_sv);

    SSLConnection *conn = malloc(sizeof(SSLConnection));
    if (!conn) return NULL;

    memset(conn, 0, sizeof(SSLConnection));
    conn->is_server = 1;

    /* Create SSL context for server */
    const SSL_METHOD *method = TLS_server_method();
    conn->ctx = SSL_CTX_new(method);
    if (!conn->ctx) {
        free(conn);
        return NULL;
    }

    /* Load certificate */
    if (SSL_CTX_use_certificate_file(conn->ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(conn->ctx);
        free(conn);
        return NULL;
    }

    /* Load private key */
    if (SSL_CTX_use_PrivateKey_file(conn->ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(conn->ctx);
        free(conn);
        return NULL;
    }

    /* Verify key matches certificate */
    if (!SSL_CTX_check_private_key(conn->ctx)) {
        SSL_CTX_free(conn->ctx);
        free(conn);
        return NULL;
    }

    /* Create socket */
    conn->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->socket_fd < 0) {
        SSL_CTX_free(conn->ctx);
        free(conn);
        return NULL;
    }

    /* Allow address reuse */
    int opt = 1;
    setsockopt(conn->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Bind to port */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(conn->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(conn->socket_fd);
        SSL_CTX_free(conn->ctx);
        free(conn);
        return NULL;
    }

    /* Listen for connections */
    if (listen(conn->socket_fd, 10) < 0) {
        close(conn->socket_fd);
        SSL_CTX_free(conn->ctx);
        free(conn);
        return NULL;
    }

    return conn;
}

/* Accept SSL connection on server socket
 * Accepts StradaValue* argument from Strada FFI */
SSLConnection* strada_ssl_accept(StradaValue *server_sv) {
    SSLConnection *server = (SSLConnection *)strada_to_int(server_sv);

    if (!server || !server->is_server) return NULL;

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(server->socket_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) return NULL;

    SSLConnection *conn = malloc(sizeof(SSLConnection));
    if (!conn) {
        close(client_fd);
        return NULL;
    }

    memset(conn, 0, sizeof(SSLConnection));
    conn->socket_fd = client_fd;
    conn->ctx = server->ctx;  /* Share context with server */
    conn->is_server = 0;

    /* Create SSL for this connection */
    conn->ssl = SSL_new(server->ctx);
    if (!conn->ssl) {
        close(client_fd);
        free(conn);
        return NULL;
    }

    SSL_set_fd(conn->ssl, client_fd);

    /* Perform SSL handshake */
    if (SSL_accept(conn->ssl) <= 0) {
        SSL_free(conn->ssl);
        close(client_fd);
        free(conn);
        return NULL;
    }

    return conn;
}

/* Read data from SSL connection */
int strada_ssl_read(SSLConnection *conn, char *buffer, int max_len) {
    if (!conn || !conn->ssl) return -1;
    return SSL_read(conn->ssl, buffer, max_len);
}

/* Read data from SSL connection, returns allocated string
 * Accepts StradaValue* arguments from Strada FFI */
char* strada_ssl_read_str(StradaValue *conn_sv, StradaValue *max_len_sv) {
    SSLConnection *conn = (SSLConnection *)strada_to_int(conn_sv);
    int max_len = (int)strada_to_int(max_len_sv);

    if (!conn || !conn->ssl) return strdup("");

    char *buffer = malloc(max_len + 1);
    if (!buffer) return strdup("");

    int n = SSL_read(conn->ssl, buffer, max_len);
    if (n <= 0) {
        free(buffer);
        return strdup("");
    }

    buffer[n] = '\0';
    return buffer;
}

/* Read line from SSL connection (up to newline or max_len)
 * Accepts StradaValue* arguments from Strada FFI */
char* strada_ssl_readline(StradaValue *conn_sv, StradaValue *max_len_sv) {
    SSLConnection *conn = (SSLConnection *)strada_to_int(conn_sv);
    int max_len = (int)strada_to_int(max_len_sv);

    if (!conn || !conn->ssl) return NULL;

    char *buffer = malloc(max_len + 1);
    if (!buffer) return NULL;

    int pos = 0;
    char c;

    while (pos < max_len) {
        int n = SSL_read(conn->ssl, &c, 1);
        if (n <= 0) break;
        buffer[pos++] = c;
        if (c == '\n') break;
    }

    buffer[pos] = '\0';
    return buffer;
}

/* Read all available data (returns allocated string, caller must free) */
char* strada_ssl_read_all(SSLConnection *conn, int max_len) {
    if (!conn || !conn->ssl) return NULL;

    char *buffer = malloc(max_len + 1);
    if (!buffer) return NULL;

    int total = 0;
    int chunk_size = 4096;

    while (total < max_len) {
        int to_read = (max_len - total < chunk_size) ? (max_len - total) : chunk_size;
        int n = SSL_read(conn->ssl, buffer + total, to_read);
        if (n <= 0) break;
        total += n;

        /* Check if more data is pending */
        if (SSL_pending(conn->ssl) == 0) {
            /* Small delay to see if more data arrives */
            break;
        }
    }

    buffer[total] = '\0';
    return buffer;
}

/* Write data to SSL connection */
int strada_ssl_write(SSLConnection *conn, const char *data, int len) {
    if (!conn || !conn->ssl) return -1;
    if (len < 0) len = strlen(data);
    return SSL_write(conn->ssl, data, len);
}

/* Write string to SSL connection
 * Accepts StradaValue* arguments from Strada FFI */
int strada_ssl_write_str(StradaValue *conn_sv, StradaValue *data_sv) {
    SSLConnection *conn = (SSLConnection *)strada_to_int(conn_sv);
    const char *data = strada_to_str(data_sv);

    if (!conn || !conn->ssl || !data) return -1;
    return SSL_write(conn->ssl, data, strlen(data));
}

/* Close SSL connection
 * Accepts StradaValue* argument from Strada FFI */
void strada_ssl_close(StradaValue *conn_sv) {
    SSLConnection *conn = (SSLConnection *)strada_to_int(conn_sv);

    if (!conn) return;

    if (conn->ssl) {
        SSL_shutdown(conn->ssl);
        SSL_free(conn->ssl);
    }

    if (conn->socket_fd >= 0) {
        close(conn->socket_fd);
    }

    /* Only free context if this is a server or standalone connection */
    if (conn->is_server && conn->ctx) {
        SSL_CTX_free(conn->ctx);
    }

    free(conn);
}

/* Get peer certificate info as string
 * Accepts StradaValue* argument from Strada FFI */
char* strada_ssl_peer_cert(StradaValue *conn_sv) {
    SSLConnection *conn = (SSLConnection *)strada_to_int(conn_sv);

    if (!conn || !conn->ssl) return strdup("");

    X509 *cert = SSL_get_peer_certificate(conn->ssl);
    if (!cert) return strdup("No certificate");

    char *subject = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0);
    X509_free(cert);

    return subject ? subject : strdup("Unknown");
}

/* Get SSL version string
 * Accepts StradaValue* argument from Strada FFI */
char* strada_ssl_version(StradaValue *conn_sv) {
    SSLConnection *conn = (SSLConnection *)strada_to_int(conn_sv);

    if (!conn || !conn->ssl) return strdup("Not connected");
    return strdup(SSL_get_version(conn->ssl));
}

/* Get cipher being used
 * Accepts StradaValue* argument from Strada FFI */
char* strada_ssl_cipher(StradaValue *conn_sv) {
    SSLConnection *conn = (SSLConnection *)strada_to_int(conn_sv);

    if (!conn || !conn->ssl) return strdup("Not connected");
    return strdup(SSL_get_cipher(conn->ssl));
}

/* Check if connection is still valid */
int strada_ssl_connected(SSLConnection *conn) {
    if (!conn || !conn->ssl) return 0;
    return SSL_get_shutdown(conn->ssl) == 0;
}

/* Get the raw socket FD (for select/poll)
 * Accepts StradaValue* argument from Strada FFI */
int strada_ssl_fd(StradaValue *conn_sv) {
    SSLConnection *conn = (SSLConnection *)strada_to_int(conn_sv);

    if (!conn) return -1;
    return conn->socket_fd;
}

/* Set socket to non-blocking mode */
int strada_ssl_set_nonblock(SSLConnection *conn, int nonblock) {
    if (!conn || conn->socket_fd < 0) return -1;

    int flags = fcntl(conn->socket_fd, F_GETFL, 0);
    if (flags < 0) return -1;

    if (nonblock) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    return fcntl(conn->socket_fd, F_SETFL, flags);
}

/* Verify server certificate (call after connect)
 * Accepts StradaValue* argument from Strada FFI */
int strada_ssl_verify(StradaValue *conn_sv) {
    SSLConnection *conn = (SSLConnection *)strada_to_int(conn_sv);

    if (!conn || !conn->ssl) return -1;

    long result = SSL_get_verify_result(conn->ssl);
    return (result == X509_V_OK) ? 0 : (int)result;
}

/* Enable certificate verification for client connections */
void strada_ssl_set_verify(SSLConnection *conn, int verify) {
    if (!conn || !conn->ctx) return;

    if (verify) {
        SSL_CTX_set_verify(conn->ctx, SSL_VERIFY_PEER, NULL);
        SSL_CTX_set_default_verify_paths(conn->ctx);
    } else {
        SSL_CTX_set_verify(conn->ctx, SSL_VERIFY_NONE, NULL);
    }
}

/* ===== StradaValue-returning wrappers for dl_call_sv ===== */

/* Create SSL server and return wrapped pointer
 * For use with sys::dl_call_sv */
StradaValue* strada_ssl_server_sv(StradaValue *port_sv, StradaValue *cert_sv, StradaValue *key_sv) {
    SSLConnection *conn = strada_ssl_server(port_sv, cert_sv, key_sv);
    if (!conn) {
        return strada_new_undef();
    }
    return strada_new_int((intptr_t)conn);
}

/* Accept SSL connection and return wrapped pointer
 * For use with sys::dl_call_sv */
StradaValue* strada_ssl_accept_sv(StradaValue *server_sv) {
    SSLConnection *conn = strada_ssl_accept(server_sv);
    if (!conn) {
        return strada_new_undef();
    }
    return strada_new_int((intptr_t)conn);
}
