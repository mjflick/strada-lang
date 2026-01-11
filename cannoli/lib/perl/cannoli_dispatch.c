/*
 * cannoli_dispatch.c - C wrapper for dispatch to handle return type conversion
 *
 * This provides the cannoli_dispatch function with the correct char* return type.
 * It calls the Strada dispatch function and extracts the string.
 *
 * New interface passes full request data for the Cannoli object.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declarations from Strada runtime */
typedef struct StradaValue StradaValue;
extern const char* strada_to_str(StradaValue *sv);
extern StradaValue* strada_from_str(const char *s);

/* Forward declaration of the Strada dispatch function (new signature) */
extern StradaValue* strada_dispatch_impl(
    StradaValue* method,
    StradaValue* path,
    StradaValue* path_info,
    StradaValue* query_string,
    StradaValue* body,
    StradaValue* headers,
    StradaValue* remote_addr,
    StradaValue* content_type
);

/* The dispatch function that cannoli calls - returns char*
 * New signature with full request data
 */
char* cannoli_dispatch(
    StradaValue* method,
    StradaValue* path,
    StradaValue* path_info,
    StradaValue* query_string,
    StradaValue* body,
    StradaValue* headers,
    StradaValue* remote_addr,
    StradaValue* content_type
) {
    /* Call the Strada dispatch implementation */
    StradaValue* result_sv = strada_dispatch_impl(
        method,
        path,
        path_info,
        query_string,
        body,
        headers,
        remote_addr,
        content_type
    );

    if (!result_sv) {
        return strdup("");
    }

    /* Extract string from StradaValue */
    const char* result = strada_to_str(result_sv);
    if (!result) {
        return strdup("");
    }

    /* Return a copy (caller will free) */
    return strdup(result);
}
