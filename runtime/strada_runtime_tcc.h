/* strada_runtime_tcc.h - Minimal header for TCC compilation */
/* This header has struct definitions but avoids problematic system includes */
#ifndef STRADA_RUNTIME_TCC_H
#define STRADA_RUNTIME_TCC_H

#include <stdint.h>
#include <stddef.h>

/* Forward declarations */
typedef struct StradaValue StradaValue;
typedef struct StradaArray StradaArray;
typedef struct StradaHash StradaHash;
typedef struct StradaHashEntry StradaHashEntry;

/* Type enumeration */
typedef enum {
    STRADA_UNDEF,
    STRADA_INT,
    STRADA_NUM,
    STRADA_STR,
    STRADA_ARRAY,
    STRADA_HASH,
    STRADA_REF,
    STRADA_FILEHANDLE,
    STRADA_REGEX,
    STRADA_SOCKET,
    STRADA_CSTRUCT,
    STRADA_CPOINTER,
    STRADA_CLOSURE,
    STRADA_FUTURE,
    STRADA_CHANNEL,
    STRADA_ATOMIC
} StradaType;

/* Opaque types we don't need full definitions for */
typedef void regex_t;
typedef void StradaSocketBuffer;
typedef void FILE;

/* Value structure - core of Strada runtime */
struct StradaValue {
    StradaType type;
    int refcount;
    union {
        int64_t iv;      /* Integer value */
        double nv;       /* Numeric value */
        char *pv;        /* String value */
        StradaArray *av; /* Array reference */
        StradaHash *hv;  /* Hash reference */
        StradaValue *rv; /* Generic reference */
        FILE *fh;        /* File handle */
        regex_t *rx;     /* Compiled regex */
        StradaSocketBuffer *sock;  /* Buffered socket */
        void *ptr;       /* Generic C pointer */
    } value;

    /* C struct metadata (when type == STRADA_CSTRUCT) */
    char *struct_name;   /* Name of C struct type */
    size_t struct_size;  /* Size of struct in bytes */

    /* Blessed package (for OOP - like Perl's bless) */
    char *blessed_package;  /* Package name this ref is blessed into, or NULL */
};

/* Array structure - like Perl's AV */
struct StradaArray {
    StradaValue **elements;
    size_t size;
    size_t capacity;
    int refcount;
};

/* Hash entry */
struct StradaHashEntry {
    char *key;
    StradaValue *value;
    struct StradaHashEntry *next;
};

/* Hash structure - like Perl's HV */
struct StradaHash {
    StradaHashEntry **buckets;
    size_t num_buckets;
    size_t num_entries;
    int refcount;
};

/* Core value creation */
StradaValue* strada_new_undef(void);
StradaValue* strada_new_int(int64_t value);
StradaValue* strada_new_num(double value);
StradaValue* strada_new_str(const char *value);
StradaValue* strada_new_str_len(const char *value, size_t len);
StradaValue* strada_new_array(void);
StradaValue* strada_new_array_with_capacity(size_t capacity);
StradaValue* strada_new_hash(void);
StradaValue* strada_new_hash_with_capacity(size_t capacity);
StradaValue* strada_new_ref(StradaValue *target, char ref_type);

/* Reference counting */
void strada_incref(StradaValue *sv);
void strada_decref(StradaValue *sv);

/* Type conversion */
int64_t strada_to_int(StradaValue *sv);
double strada_to_num(StradaValue *sv);
const char* strada_to_str(StradaValue *sv);
int strada_to_bool(StradaValue *sv);

/* String comparison with C literals (no temporaries) */
int strada_str_eq_lit(StradaValue *sv, const char *lit);
int strada_str_ne_lit(StradaValue *sv, const char *lit);
int strada_str_lt_lit(StradaValue *sv, const char *lit);
int strada_str_gt_lit(StradaValue *sv, const char *lit);
int strada_str_le_lit(StradaValue *sv, const char *lit);
int strada_str_ge_lit(StradaValue *sv, const char *lit);

/* Type checking */
int strada_is_true(StradaValue *sv);
int strada_is_defined(StradaValue *sv);
const char* strada_typeof(StradaValue *sv);

/* String operations */
StradaValue* strada_str_concat(StradaValue *a, StradaValue *b);
StradaValue* strada_concat_sv(StradaValue *a, StradaValue *b);  /* Fast concat on StradaValues */
StradaValue* strada_str_repeat(StradaValue *s, StradaValue *n);
int64_t strada_str_length(StradaValue *sv);
StradaValue* strada_substr(StradaValue *sv, int64_t start, int64_t len);
int64_t strada_index(StradaValue *haystack, StradaValue *needle, int64_t start);
StradaValue* strada_uc(StradaValue *sv);
StradaValue* strada_lc(StradaValue *sv);
StradaValue* strada_ucfirst(StradaValue *sv);
StradaValue* strada_lcfirst(StradaValue *sv);
StradaValue* strada_chr(StradaValue *n);
StradaValue* strada_ord(StradaValue *s);
StradaValue* strada_sprintf(StradaValue *fmt, StradaValue *args);
StradaValue* strada_join(StradaValue *sep, StradaValue *arr);
StradaValue* strada_split(StradaValue *pattern, StradaValue *str, StradaValue *limit);
StradaValue* strada_char_at(StradaValue *sv, int64_t idx);

/* Array operations */
void strada_array_push(StradaArray *arr, StradaValue *val);
StradaValue* strada_array_pop(StradaArray *arr);
StradaValue* strada_array_shift(StradaArray *arr);
void strada_array_unshift(StradaArray *arr, StradaValue *val);
StradaValue* strada_array_get(StradaArray *arr, int64_t index);
void strada_array_set(StradaArray *arr, int64_t index, StradaValue *val);
int64_t strada_array_length(StradaArray *arr);
void strada_array_reverse(StradaArray *arr);
StradaValue* strada_array_slice(StradaValue *arr, StradaValue *start, StradaValue *end);
StradaValue* strada_array_splice(StradaValue *arr, StradaValue *offset, StradaValue *length, StradaValue *replacement);

/* Hash operations */
StradaValue* strada_hash_get(StradaHash *hash, const char *key);
void strada_hash_set(StradaHash *hash, const char *key, StradaValue *val);
int strada_hash_exists(StradaHash *hash, const char *key);
void strada_hash_delete(StradaHash *hash, const char *key);
StradaValue* strada_hash_keys(StradaHash *hash);
StradaValue* strada_hash_values(StradaHash *hash);
int64_t strada_hash_size(StradaHash *hash);

/* Reference operations */
StradaValue* strada_deref(StradaValue *ref);
StradaArray* strada_deref_array(StradaValue *ref);
StradaHash* strada_deref_hash(StradaValue *ref);
StradaValue* strada_deref_array_value(StradaValue *ref);
StradaValue* strada_deref_hash_value(StradaValue *ref);

/* Comparison */
int strada_num_cmp(StradaValue *a, StradaValue *b);
int strada_str_cmp(StradaValue *a, StradaValue *b);
int strada_str_eq(StradaValue *a, StradaValue *b);
int strada_str_ne(StradaValue *a, StradaValue *b);
int strada_str_lt(StradaValue *a, StradaValue *b);
int strada_str_gt(StradaValue *a, StradaValue *b);
int strada_str_le(StradaValue *a, StradaValue *b);
int strada_str_ge(StradaValue *a, StradaValue *b);

/* I/O - Console */
void strada_say(StradaValue *sv);
void strada_print(StradaValue *sv);
void strada_warn(StradaValue *sv);
void strada_die(StradaValue *sv);

/* I/O - File operations */
StradaValue* strada_open(const char *filename, const char *mode);
void strada_close(StradaValue *fh);
StradaValue* strada_read_line(StradaValue *fh);
StradaValue* strada_read_all_lines(StradaValue *fh);
StradaValue* strada_slurp(const char *filename);
StradaValue* strada_slurp_fh(StradaValue *fh_sv);
StradaValue* strada_slurp_fd(StradaValue *fd_sv);
void strada_spew(const char *filename, const char *content);
void strada_spew_fh(StradaValue *fh_sv, StradaValue *content_sv);
void strada_spew_fd(StradaValue *fd_sv, StradaValue *content_sv);
void strada_write_file(StradaValue *fh, const char *content);
int strada_file_exists(const char *filename);

/* Regex */
StradaValue* strada_regex_match(StradaValue *str, StradaValue *pattern, StradaValue *flags);
StradaValue* strada_regex_subst(StradaValue *str, StradaValue *pattern, StradaValue *replacement, StradaValue *flags);

/* OOP */
StradaValue* strada_bless(StradaValue *ref, StradaValue *classname);
StradaValue* strada_blessed(StradaValue *ref);
int strada_isa(StradaValue *obj, const char *classname);
void strada_register_method(const char *classname, const char *methodname, void *func);
void strada_method_register(const char *classname, const char *methodname, void *func);
StradaValue* strada_method_call(StradaValue *obj, const char *method, StradaValue *args);
StradaValue* strada_can(StradaValue *obj, const char *method);

/* Closures */
StradaValue* strada_closure_new(void *func, int arg_count, int capture_count, StradaValue ***captures);
StradaValue* strada_closure_call(StradaValue *closure, StradaValue *args);
StradaValue* strada_closure_call_method(StradaValue *closure, StradaValue *self, StradaValue *args);

/* Exceptions */
void strada_throw(const char *msg);
void strada_throw_value(StradaValue *val);
StradaValue* strada_get_exception(void);

/* Math */
double strada_math_sin(double x);
double strada_math_cos(double x);
double strada_math_tan(double x);
double strada_math_sqrt(double x);
double strada_math_pow(double x, double y);
double strada_math_log(double x);
double strada_math_exp(double x);
double strada_math_floor(double x);
double strada_math_ceil(double x);
double strada_math_abs(double x);
int64_t strada_math_rand(void);
void strada_math_srand(int64_t seed);

/* Defined check */
StradaValue* strada_defined(StradaValue *sv);

/* Scalar function (get count) */
int64_t strada_scalar(StradaValue *sv);

/* Wantarray (context) */
int strada_wantarray(void);

/* Exists/delete for hashes */
int strada_exists(StradaValue *hash, StradaValue *key);
void strada_delete(StradaValue *hash, StradaValue *key);

/* Debugging/inspection */
void strada_dumper(StradaValue *sv);
StradaValue* strada_dumper_str(StradaValue *sv);

/* Anonymous constructors */
StradaValue* strada_anon_hash(int count, ...);
StradaValue* strada_anon_array(int count, ...);

/* Memory profiling */
void strada_memprof_enable(void);
void strada_memprof_disable(void);
void strada_memprof_reset(void);
void strada_memprof_report(void);

/* Function profiling */
void strada_profile_init(void);
void strada_profile_enter(const char *func_name);
void strada_profile_exit(const char *func_name);
void strada_profile_report(void);

#endif /* STRADA_RUNTIME_TCC_H */
