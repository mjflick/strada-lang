/*
 * lib_api.c - Public API library for multi-library testing
 * Routes: /api, /api/*
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct StradaValue StradaValue;
extern const char* strada_to_str(StradaValue* v);

static int starts_with(const char* str, const char* prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

const char* cannoli_dispatch(StradaValue* method_sv, StradaValue* path_sv,
                             StradaValue* path_info_sv, StradaValue* body_sv) {
    static char response[4096];

    const char* method = strada_to_str(method_sv);
    const char* path = strada_to_str(path_sv);
    const char* path_info = strada_to_str(path_info_sv);

    /* Only handle /api routes (but not /admin) */
    if (!starts_with(path, "/api")) {
        return "";  /* Pass to next library */
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api") == 0) {
        return "{\"service\":\"api\",\"version\":\"1.0\",\"routes\":[\"/api/users\",\"/api/products\"]}";
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/users") == 0) {
        return "[{\"id\":1,\"name\":\"Alice\"},{\"id\":2,\"name\":\"Bob\"}]";
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/products") == 0) {
        return "[{\"id\":100,\"name\":\"Widget\"},{\"id\":101,\"name\":\"Gadget\"}]";
    }

    /* /api/* fallback */
    if (starts_with(path, "/api/")) {
        snprintf(response, sizeof(response),
            "{\"error\":\"api route not found\",\"path\":\"%s\",\"path_info\":\"%s\"}",
            path, path_info);
        return response;
    }

    return "";
}
