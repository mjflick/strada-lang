/*
 * lib_pathinfo.c - Library to test path_info functionality
 */

#include <stdio.h>
#include <string.h>

typedef struct StradaValue StradaValue;
extern const char* strada_to_str(StradaValue* v);

const char* cannoli_dispatch(StradaValue* method_sv, StradaValue* path_sv,
                             StradaValue* path_info_sv, StradaValue* body_sv) {
    static char response[4096];

    const char* method = strada_to_str(method_sv);
    const char* path = strada_to_str(path_sv);
    const char* path_info = strada_to_str(path_info_sv);

    /* Always respond to show what we received */
    snprintf(response, sizeof(response),
        "{\"method\":\"%s\",\"path\":\"%s\",\"path_info\":\"%s\",\"library\":\"pathinfo_test\"}",
        method, path, path_info);
    return response;
}
