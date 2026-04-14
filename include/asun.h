/*
 * ASUN - A Schema-Oriented Notation (C Implementation)
 *
 * High-performance, zero-copy ASUN serializer/deserializer for C.
 * Uses SIMD (NEON/SSE2) for accelerated string scanning.
 *
 * Usage pattern (macro-based reflection):
 *
 *   typedef struct { int64_t id; char* name; bool active; } User;
 *   ASUN_FIELDS(User, 3,
 *     ASUN_FIELD(User, id,     "id",     ASUN_I64),
 *     ASUN_FIELD(User, name,   "name",   ASUN_STR),
 *     ASUN_FIELD(User, active, "active", ASUN_BOOL))
 *
 *   asun_buf_t buf = asun_encode_User(&user);
 *   User u2 = {0}; asun_decode_User(buf.data, buf.len, &u2);
 */

#ifndef ASUN_H
#define ASUN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Platform / SIMD detection
 * ============================================================================ */

#if defined(__aarch64__) || defined(_M_ARM64)
  #define ASUN_NEON 1
  #include <arm_neon.h>
#elif defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64)
  #define ASUN_SSE2 1
  #include <emmintrin.h>
#endif

/* ============================================================================
 * Compiler hints
 * ============================================================================ */

#if defined(__GNUC__) || defined(__clang__)
  #define asun_likely(x)   __builtin_expect(!!(x), 1)
  #define asun_unlikely(x) __builtin_expect(!!(x), 0)
  #define asun_inline      static inline __attribute__((always_inline))
  #define asun_noinline    __attribute__((noinline))
#elif defined(_MSC_VER)
  #define asun_likely(x)   (x)
  #define asun_unlikely(x) (x)
  #define asun_inline      static __forceinline
  #define asun_noinline    __declspec(noinline)
#else
  #define asun_likely(x)   (x)
  #define asun_unlikely(x) (x)
  #define asun_inline      static inline
  #define asun_noinline
#endif

/* ============================================================================
 * Error codes
 * ============================================================================ */

typedef enum {
    ASUN_OK = 0,
    ASUN_ERR_ALLOC,
    ASUN_ERR_SYNTAX,
    ASUN_ERR_UNEXPECTED_CHAR,
    ASUN_ERR_INVALID_NUMBER,
    ASUN_ERR_BUFFER_OVERFLOW,
    ASUN_ERR_SCHEMA_MISMATCH,
    ASUN_ERR_MISSING_FIELD,
} asun_err_t;

/* ============================================================================
 * Resizable buffer (serialization output)
 * ============================================================================ */

typedef struct {
    char*  data;
    size_t len;
    size_t cap;
} asun_buf_t;

asun_inline asun_buf_t asun_buf_new(size_t init_cap) {
    asun_buf_t b;
    b.cap = init_cap < 64 ? 64 : init_cap;
    b.data = (char*)malloc(b.cap);
    b.len = 0;
    return b;
}

asun_inline void asun_buf_free(asun_buf_t* b) {
    if (b->data) { free(b->data); b->data = NULL; }
    b->len = b->cap = 0;
}

asun_inline void asun_buf_grow(asun_buf_t* b, size_t need) {
    if (asun_likely(b->len + need <= b->cap)) return;
    size_t nc = b->cap;
    while (nc < b->len + need) nc = nc + (nc >> 1);
    b->data = (char*)realloc(b->data, nc);
    b->cap = nc;
}

asun_inline void asun_buf_push(asun_buf_t* b, char c) {
    asun_buf_grow(b, 1);
    b->data[b->len++] = c;
}

asun_inline void asun_buf_append(asun_buf_t* b, const char* s, size_t n) {
    asun_buf_grow(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
}

asun_inline void asun_buf_appends(asun_buf_t* b, const char* s) {
    asun_buf_append(b, s, strlen(s));
}

/* ============================================================================
 * Dynamic string (heap-allocated, owning)
 * ============================================================================ */

typedef struct {
    char*  data;
    size_t len;
} asun_string_t;

asun_inline asun_string_t asun_string_new(const char* s, size_t n) {
    asun_string_t r;
    r.data = (char*)malloc(n + 1);
    memcpy(r.data, s, n);
    r.data[n] = '\0';
    r.len = n;
    return r;
}

asun_inline asun_string_t asun_string_from(const char* s) {
    return asun_string_new(s, strlen(s));
}

asun_inline asun_string_t asun_string_from_len(const char* s, size_t n) {
    return asun_string_new(s, n);
}

asun_inline void asun_string_free(asun_string_t* s) {
    if (s->data) { free(s->data); s->data = NULL; }
    s->len = 0;
}

/* ============================================================================
 * Dynamic array (generic via macros)
 * ============================================================================ */

#define ASUN_VEC_DEFINE(name, T) \
    typedef struct { T* data; size_t len; size_t cap; } name; \
    asun_inline name name##_new(void) { \
        name v; v.data = NULL; v.len = v.cap = 0; return v; \
    } \
    asun_inline void name##_push(name* v, T val) { \
        if (v->len >= v->cap) { \
            v->cap = v->cap < 4 ? 4 : v->cap + (v->cap >> 1); \
            v->data = (T*)realloc(v->data, v->cap * sizeof(T)); \
        } \
        v->data[v->len++] = val; \
    } \
    asun_inline void name##_free(name* v) { \
        if (v->data) free(v->data); \
        v->data = NULL; v->len = v->cap = 0; \
    }

/* Pre-defined vec types */
ASUN_VEC_DEFINE(asun_vec_i64, int64_t)
ASUN_VEC_DEFINE(asun_vec_u64, uint64_t)
ASUN_VEC_DEFINE(asun_vec_f64, double)
ASUN_VEC_DEFINE(asun_vec_str, asun_string_t)
ASUN_VEC_DEFINE(asun_vec_bool, bool)

/* Nested vec */
ASUN_VEC_DEFINE(asun_vec_vec_i64, asun_vec_i64)

/* ============================================================================
 * Optional (has_value + value)
 * ============================================================================ */

typedef struct { bool has_value; int64_t value; } asun_opt_i64;
typedef struct { bool has_value; asun_string_t value; } asun_opt_str;
typedef struct { bool has_value; double value; } asun_opt_f64;

/* ============================================================================
 * Field type enum
 * ============================================================================ */

typedef enum {
    ASUN_BOOL = 0,
    ASUN_I8, ASUN_I16, ASUN_I32, ASUN_I64,
    ASUN_U8, ASUN_U16, ASUN_U32, ASUN_U64,
    ASUN_F32, ASUN_F64,
    ASUN_CHAR,
    ASUN_STR,
    ASUN_OPT_I64, ASUN_OPT_STR, ASUN_OPT_F64,
    ASUN_VEC_I64, ASUN_VEC_U64, ASUN_VEC_F64,
    ASUN_VEC_STR, ASUN_VEC_BOOL,
    ASUN_VEC_VEC_I64,
    ASUN_STRUCT,
} asun_type_t;

/* ============================================================================
 * Field descriptor
 * ============================================================================ */

typedef void (*asun_encode_fn)(asun_buf_t* buf, const void* base, size_t offset);
typedef asun_err_t (*asun_decode_fn)(const char** pos, const char* end, void* base, size_t offset);

typedef struct {
    const char*  name;
    size_t       name_len;
    asun_type_t  type;
    size_t       offset;
    const char*  type_str;
    /* For ASUN_STRUCT fields: pointer to sub-struct descriptor */
    const void*  sub_desc;
    asun_encode_fn dump_fn;
    asun_decode_fn load_fn;
} asun_field_t;

/* ============================================================================
 * Struct descriptor
 * ============================================================================ */

typedef struct {
    const char*       struct_name;
    const asun_field_t* fields;
    int               field_count;
} asun_desc_t;

/* ============================================================================
 * SIMD-accelerated helpers
 * ============================================================================ */

/* Find first occurrence of '"' or '\\' or control char in [pos, end).
 * Returns pointer to the found char, or end if none. */
asun_inline const char* asun_find_quote_or_special(const char* pos, const char* end) {
#if defined(ASUN_NEON)
    uint8x16_t q_dq  = vdupq_n_u8('"');
    uint8x16_t q_bs  = vdupq_n_u8('\\');
    uint8x16_t q_lim = vdupq_n_u8(0x20);
    while (pos + 16 <= end) {
        uint8x16_t chunk = vld1q_u8((const uint8_t*)pos);
        uint8x16_t m = vorrq_u8(
            vorrq_u8(vceqq_u8(chunk, q_dq), vceqq_u8(chunk, q_bs)),
            vcltq_u8(chunk, q_lim));
        /* Reduce to scalar: any lane set? */
        if (vmaxvq_u8(m)) {
            for (int i = 0; i < 16; i++)
                if (pos[i] == '"' || pos[i] == '\\' || (uint8_t)pos[i] < 0x20) return pos + i;
        }
        pos += 16;
    }
#elif defined(ASUN_SSE2)
    __m128i q_dq  = _mm_set1_epi8('"');
    __m128i q_bs  = _mm_set1_epi8('\\');
    __m128i q_lim = _mm_set1_epi8(0x1F);
    while (pos + 16 <= end) {
        __m128i chunk = _mm_loadu_si128((const __m128i*)pos);
        __m128i m = _mm_or_si128(
            _mm_or_si128(_mm_cmpeq_epi8(chunk, q_dq), _mm_cmpeq_epi8(chunk, q_bs)),
            _mm_cmpeq_epi8(_mm_min_epu8(chunk, q_lim), chunk));
        int mask = _mm_movemask_epi8(m);
        if (mask) {
            int idx = __builtin_ctz(mask);
            return pos + idx;
        }
        pos += 16;
    }
#endif
    /* Scalar fallback */
    while (pos < end) {
        if (*pos == '"' || *pos == '\\' || (uint8_t)*pos < 0x20) return pos;
        pos++;
    }
    return end;
}

/* Check if string needs quoting (contains structural chars) */
asun_inline bool asun_needs_quoting(const char* s, size_t len) {
    if (len == 0) return true;
    /* Leading/trailing whitespace */
    if (s[0] == ' ' || s[0] == '\t' || s[len-1] == ' ' || s[len-1] == '\t') return true;
    /* Bool/number-like */
    if (len == 4 &&
        ((s[0] == 't' && s[1] == 'r' && s[2] == 'u' && s[3] == 'e') ||
         (s[0] == 'n' && s[1] == 'u' && s[2] == 'l' && s[3] == 'l'))) return true;
    if (len == 5 &&
        s[0] == 'f' && s[1] == 'a' && s[2] == 'l' && s[3] == 's' && s[4] == 'e') return true;
    if ((s[0] >= '0' && s[0] <= '9') || s[0] == '-' || s[0] == '+') return true;

#if defined(ASUN_NEON)
    /* SIMD: check for structural chars: ,@()[]{}:"\\<ctrl> */
    uint8x16_t q_comma = vdupq_n_u8(',');
    uint8x16_t q_at    = vdupq_n_u8('@');
    uint8x16_t q_lp    = vdupq_n_u8('(');
    uint8x16_t q_rp    = vdupq_n_u8(')');
    uint8x16_t q_lb    = vdupq_n_u8('[');
    uint8x16_t q_rb    = vdupq_n_u8(']');
    uint8x16_t q_lc    = vdupq_n_u8('{');
    uint8x16_t q_rc    = vdupq_n_u8('}');
    uint8x16_t q_dq    = vdupq_n_u8('"');
    uint8x16_t q_colon = vdupq_n_u8(':');
    uint8x16_t q_bs    = vdupq_n_u8('\\');
    uint8x16_t q_lim   = vdupq_n_u8(0x20);
    const char* p = s;
    const char* e = s + len;
    while (p + 16 <= e) {
        uint8x16_t chunk = vld1q_u8((const uint8_t*)p);
        uint8x16_t m = vorrq_u8(
            vorrq_u8(vorrq_u8(vorrq_u8(vceqq_u8(chunk, q_comma), vceqq_u8(chunk, q_at)), vceqq_u8(chunk, q_lp)),
                     vorrq_u8(vceqq_u8(chunk, q_rp), vceqq_u8(chunk, q_lb))),
            vorrq_u8(vorrq_u8(vceqq_u8(chunk, q_rb), vceqq_u8(chunk, q_lc)),
                     vorrq_u8(vorrq_u8(vceqq_u8(chunk, q_rc), vceqq_u8(chunk, q_dq)),
                              vorrq_u8(vorrq_u8(vceqq_u8(chunk, q_colon), vceqq_u8(chunk, q_bs)),
                                       vcltq_u8(chunk, q_lim)))));
        if (vmaxvq_u8(m)) return true;
        p += 16;
    }
    while (p < e) {
        char c = *p;
        if (c == ',' || c == '@' || c == '(' || c == ')' || c == '[' || c == ']' ||
            c == '{' || c == '}' || c == '"' || c == ':' || c == '\\' ||
            (uint8_t)c < 0x20) return true;
        p++;
    }
#elif defined(ASUN_SSE2)
    __m128i q_comma = _mm_set1_epi8(',');
    __m128i q_at    = _mm_set1_epi8('@');
    __m128i q_lp    = _mm_set1_epi8('(');
    __m128i q_rp    = _mm_set1_epi8(')');
    __m128i q_lb    = _mm_set1_epi8('[');
    __m128i q_rb    = _mm_set1_epi8(']');
    __m128i q_lc    = _mm_set1_epi8('{');
    __m128i q_rc    = _mm_set1_epi8('}');
    __m128i q_dq    = _mm_set1_epi8('"');
    __m128i q_colon = _mm_set1_epi8(':');
    __m128i q_bs    = _mm_set1_epi8('\\');
    __m128i q_lim   = _mm_set1_epi8(0x1F);
    const char* p = s;
    const char* e = s + len;
    while (p + 16 <= e) {
        __m128i chunk = _mm_loadu_si128((const __m128i*)p);
        __m128i m = _mm_or_si128(
            _mm_or_si128(_mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(chunk, q_comma), _mm_cmpeq_epi8(chunk, q_at)), _mm_cmpeq_epi8(chunk, q_lp)),
                         _mm_or_si128(_mm_cmpeq_epi8(chunk, q_rp), _mm_cmpeq_epi8(chunk, q_lb))),
            _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(chunk, q_rb), _mm_cmpeq_epi8(chunk, q_lc)),
                         _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(chunk, q_rc), _mm_cmpeq_epi8(chunk, q_dq)),
                                      _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(chunk, q_colon), _mm_cmpeq_epi8(chunk, q_bs)),
                                                   _mm_cmpeq_epi8(_mm_min_epu8(chunk, q_lim), chunk)))));
        if (_mm_movemask_epi8(m)) return true;
        p += 16;
    }
    while (p < e) {
        char c = *p;
        if (c == ',' || c == '@' || c == '(' || c == ')' || c == '[' || c == ']' ||
            c == '{' || c == '}' || c == '"' || c == ':' || c == '\\' ||
            (uint8_t)c < 0x20) return true;
        p++;
    }
#else
    const char* p = s;
    const char* e = s + len;
    while (p < e) {
        char c = *p;
        if (c == ',' || c == '@' || c == '(' || c == ')' || c == '[' || c == ']' ||
            c == '{' || c == '}' || c == '"' || c == ':' || c == '\\' ||
            (uint8_t)c < 0x20) return true;
        p++;
    }
#endif
    return false;
}

/* ============================================================================
 * Fast number formatting (DEC_DIGITS lookup, same as C++ version)
 * ============================================================================ */

static const char ASUN_DEC_DIGITS[201] =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";

asun_inline void asun_buf_append_u64(asun_buf_t* b, uint64_t v) {
    char tmp[20];
    int i = 20;
    if (v == 0) { asun_buf_push(b, '0'); return; }
    while (v >= 100) {
        int idx = (int)(v % 100) * 2;
        v /= 100;
        tmp[--i] = ASUN_DEC_DIGITS[idx + 1];
        tmp[--i] = ASUN_DEC_DIGITS[idx];
    }
    if (v >= 10) {
        int idx = (int)v * 2;
        tmp[--i] = ASUN_DEC_DIGITS[idx + 1];
        tmp[--i] = ASUN_DEC_DIGITS[idx];
    } else {
        tmp[--i] = (char)('0' + v);
    }
    asun_buf_append(b, tmp + i, 20 - i);
}

asun_inline void asun_buf_append_i64(asun_buf_t* b, int64_t v) {
    if (v < 0) {
        asun_buf_push(b, '-');
        asun_buf_append_u64(b, (uint64_t)(-(v + 1)) + 1);
    } else {
        asun_buf_append_u64(b, (uint64_t)v);
    }
}

asun_inline void asun_buf_append_f64(asun_buf_t* b, double v) {
    if (v != v) { asun_buf_appends(b, "NaN"); return; }
    if (v == 1.0/0.0) { asun_buf_appends(b, "Inf"); return; }
    if (v == -1.0/0.0) { asun_buf_appends(b, "-Inf"); return; }
    if (v < 0) { asun_buf_push(b, '-'); v = -v; }
    double intpart, fracpart;
    fracpart = modf(v, &intpart);
    if (fracpart == 0.0 && intpart < 1e15) {
        asun_buf_append_u64(b, (uint64_t)intpart);
        asun_buf_appends(b, ".0");
        return;
    }
    /* Check 1-decimal / 2-decimal fast paths */
    double f1 = fracpart * 10.0;
    double r1 = f1 - (int)f1;
    if (r1 < 1e-9 && intpart < 1e15) {
        asun_buf_append_u64(b, (uint64_t)intpart);
        asun_buf_push(b, '.');
        asun_buf_push(b, '0' + (int)f1);
        return;
    }
    double f2 = fracpart * 100.0;
    double r2 = f2 - (int)f2;
    if (r2 < 1e-9 && intpart < 1e15) {
        asun_buf_append_u64(b, (uint64_t)intpart);
        asun_buf_push(b, '.');
        int d = (int)f2;
        asun_buf_push(b, '0' + d / 10);
        asun_buf_push(b, '0' + d % 10);
        return;
    }
    /* Fallback */
    char tmp[64];
    int n = snprintf(tmp, sizeof(tmp), "%.17g", (v < 0 ? -v : v));
    /* Ensure decimal point */
    bool has_dot = false;
    for (int i = 0; i < n; i++) { if (tmp[i] == '.' || tmp[i] == 'e' || tmp[i] == 'E') { has_dot = true; break; } }
    asun_buf_append(b, tmp, n);
    if (!has_dot) asun_buf_appends(b, ".0");
}

/* ============================================================================
 * String escaping (write)
 * ============================================================================ */

static const char ASUN_ESCAPE[256] = {
    ['\\'] = '\\', ['"'] = '"', ['\n'] = 'n', ['\r'] = 'r', ['\t'] = 't',
    ['\b'] = 'b', ['\f'] = 'f',
};

asun_inline void asun_buf_append_escaped(asun_buf_t* b, const char* s, size_t len) {
    asun_buf_push(b, '"');
    const char* pos = s;
    const char* end = s + len;
    while (pos < end) {
        const char* next = asun_find_quote_or_special(pos, end);
        if (next > pos) asun_buf_append(b, pos, next - pos);
        if (next >= end) break;
        char c = *next;
        char esc = ASUN_ESCAPE[(uint8_t)c];
        if (esc) {
            asun_buf_push(b, '\\');
            asun_buf_push(b, esc);
        } else if ((uint8_t)c < 0x20) {
            char hex[7];
            snprintf(hex, sizeof(hex), "\\u%04x", (uint8_t)c);
            asun_buf_append(b, hex, 6);
        } else {
            asun_buf_push(b, c);
        }
        pos = next + 1;
    }
    asun_buf_push(b, '"');
}

asun_inline void asun_buf_append_str(asun_buf_t* b, const char* s, size_t len) {
    if (asun_needs_quoting(s, len)) {
        asun_buf_append_escaped(b, s, len);
    } else {
        asun_buf_append(b, s, len);
    }
}

/* ============================================================================
 * Parser helpers
 * ============================================================================ */

asun_inline void asun_skip_ws(const char** pos, const char* end) {
    while (*pos < end) {
        char c = **pos;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { (*pos)++; continue; }
        /* Comment: /asterisk ... asterisk/ */
        if (c == '/' && *pos + 1 < end && (*pos)[1] == '*') {
            *pos += 2;
            while (*pos + 1 < end) {
                if (**pos == '*' && (*pos)[1] == '/') { *pos += 2; break; }
                (*pos)++;
            }
            continue;
        }
        break;
    }
}

asun_inline bool asun_at_value_end(const char* pos, const char* end) {
    if (pos >= end) return true;
    char c = *pos;
    return c == ',' || c == ')' || c == ']';
}

/* Parse a quoted string. Zero-copy: returns pointers into a decode buffer. */
asun_inline asun_err_t asun_parse_quoted_string(const char** pos, const char* end,
                                                 char** out, size_t* out_len) {
    const char* p = *pos;
    if (p >= end || *p != '"') return ASUN_ERR_SYNTAX;
    p++;
    /* Fast path: scan for end quote with SIMD */
    const char* start = p;
    const char* found = asun_find_quote_or_special(p, end);
    if (found < end && *found == '"' && found > start) {
        /* No escapes — zero-copy */
        *out = (char*)start;
        *out_len = found - start;
        *pos = found + 1;
        return ASUN_OK;
    }
    /* Slow path: has escapes */
    size_t cap = 64;
    char* buf = (char*)malloc(cap);
    size_t len = 0;
    /* Copy any prefix before the special char */
    if (found > start) {
        len = found - start;
        if (len > cap) { cap = len * 2; buf = (char*)realloc(buf, cap); }
        memcpy(buf, start, len);
    }
    p = found;
    while (p < end) {
        if (*p == '"') { p++; break; }
        if (*p == '\\') {
            p++;
            if (p >= end) { free(buf); return ASUN_ERR_SYNTAX; }
            char c = *p++;
            if (len >= cap) { cap = cap * 2; buf = (char*)realloc(buf, cap); }
            switch (c) {
                case 'n': buf[len++] = '\n'; break;
                case 'r': buf[len++] = '\r'; break;
                case 't': buf[len++] = '\t'; break;
                case 'b': buf[len++] = '\b'; break;
                case 'f': buf[len++] = '\f'; break;
                case '\\': buf[len++] = '\\'; break;
                case '"': buf[len++] = '"'; break;
                case '/': buf[len++] = '/'; break;
                default: buf[len++] = c; break;
            }
        } else {
            if (len >= cap) { cap = cap * 2; buf = (char*)realloc(buf, cap); }
            buf[len++] = *p++;
        }
    }
    buf[len] = '\0';
    *out = buf;
    *out_len = len;
    *pos = p;
    return ASUN_OK;
}

/* Parse a plain (unquoted) value, trimming whitespace */
asun_inline asun_err_t asun_parse_plain_value(const char** pos, const char* end,
                                               char** out, size_t* out_len) {
    const char* start = *pos;
    const char* p = start;
    /* Scan to delimiter and check for backslash in one pass */
    bool has_esc = false;
    while (p < end) {
        char c = *p;
        if (c == ',' || c == ')' || c == ']') break;
        if (c == '\\') has_esc = true;
        p++;
    }
    /* Trim trailing whitespace */
    const char* vend = p;
    while (vend > start && (vend[-1] == ' ' || vend[-1] == '\t' || vend[-1] == '\n' || vend[-1] == '\r')) vend--;
    size_t len = vend - start;
    if (has_esc) {
        char* buf = (char*)malloc(len + 1);
        size_t j = 0;
        for (const char* c = start; c < vend; c++) {
            if (*c == '\\' && c + 1 < vend) {
                c++;
                switch (*c) {
                    case 'n': buf[j++] = '\n'; break;
                    case 'r': buf[j++] = '\r'; break;
                    case 't': buf[j++] = '\t'; break;
                    case '\\': buf[j++] = '\\'; break;
                    case '"': buf[j++] = '"'; break;
                    default: buf[j++] = *c; break;
                }
            } else {
                buf[j++] = *c;
            }
        }
        buf[j] = '\0';
        *out = buf;
        *out_len = j;
    } else {
        *out = (char*)start; /* zero-copy */
        *out_len = len;
    }
    *pos = p;
    return ASUN_OK;
}

/* Parse a string value (quoted or plain) */
asun_inline asun_err_t asun_parse_string_value(const char** pos, const char* end,
                                                char** out, size_t* out_len,
                                                bool* allocated) {
    asun_skip_ws(pos, end);
    *allocated = false;
    if (*pos < end && **pos == '"') {
        const char* before = *pos;
        asun_err_t err = asun_parse_quoted_string(pos, end, out, out_len);
        if (err != ASUN_OK) return err;
        /* Detect if result was heap-allocated (slow path with escapes) */
        /* Zero-copy returns pointer into input [before+1..*pos-1) */
        *allocated = (*out < before || *out >= *pos);
        return ASUN_OK;
    }
    const char* before = *pos;
    asun_err_t err = asun_parse_plain_value(pos, end, out, out_len);
    if (err != ASUN_OK) return err;
    /* Detect if result was heap-allocated (has_esc path) */
    *allocated = (*out < before || *out >= *pos);
    return err;
}

/* Skip a balanced value without decoding */
asun_inline void asun_skip_balanced(const char** pos, const char* end, char open, char close) {
    int depth = 1;
    (*pos)++;
    while (*pos < end && depth > 0) {
        char c = **pos;
        if (c == open) depth++;
        else if (c == close) depth--;
        else if (c == '"') {
            (*pos)++;
            while (*pos < end && **pos != '"') {
                if (**pos == '\\') (*pos)++;
                (*pos)++;
            }
            if (*pos < end) (*pos)++;
            continue;
        }
        (*pos)++;
    }
}

asun_inline void asun_skip_value(const char** pos, const char* end) {
    asun_skip_ws(pos, end);
    if (*pos >= end) return;
    char c = **pos;
    if (c == '(') { asun_skip_balanced(pos, end, '(', ')'); return; }
    if (c == '[') { asun_skip_balanced(pos, end, '[', ']'); return; }
    if (c == '{') { asun_skip_balanced(pos, end, '{', '}'); return; }
    if (c == '"') {
        (*pos)++;
        while (*pos < end && **pos != '"') {
            if (**pos == '\\') (*pos)++;
            (*pos)++;
        }
        if (*pos < end) (*pos)++;
        return;
    }
    while (*pos < end && **pos != ',' && **pos != ')' && **pos != ']') (*pos)++;
}

/* Skip remaining comma-separated values in a tuple until ')' is found.
 * Used when the target struct has fewer fields than the source data. */
asun_inline void asun_skip_remaining_tuple_values(const char** pos, const char* end) {
    for (;;) {
        asun_skip_ws(pos, end);
        if (*pos >= end || **pos == ')') return;
        if (**pos == ',') { (*pos)++; asun_skip_ws(pos, end); if (*pos >= end || **pos == ')') return; }
        else return;
        asun_skip_value(pos, end);
    }
}

/* Parse the schema: {field1,field2,...} or {field1@type1,...} */
/* Returns field names (just pointers + lengths into the input). */
typedef struct { const char* name; size_t len; bool allocated; } asun_schema_field_t;

static asun_err_t asun_parse_schema(const char** pos, const char* end,
                                    asun_schema_field_t* fields, int* count, int max_fields);
static void asun_free_schema_fields(asun_schema_field_t* fields, int count);

static asun_err_t asun_validate_schema_scalar_type(const char** pos, const char* end) {
    const char* start = *pos;
    while (*pos < end && **pos != ',' && **pos != '}' && **pos != ']' &&
           **pos != ' ' && **pos != '\t') (*pos)++;
    size_t len = *pos - start;
    if (len == 0) return ASUN_ERR_SYNTAX;
    if (start[len - 1] == '?') len--;
    if ((len == 3 && memcmp(start, "int", 3) == 0) ||
        (len == 3 && memcmp(start, "str", 3) == 0) ||
        (len == 4 && memcmp(start, "bool", 4) == 0) ||
        (len == 5 && memcmp(start, "float", 5) == 0)) {
        return ASUN_OK;
    }
    return ASUN_ERR_SYNTAX;
}

static asun_err_t asun_validate_schema_annotation(const char** pos, const char* end) {
    if (*pos >= end) return ASUN_ERR_SYNTAX;
    if (**pos == '{') {
        asun_schema_field_t nested_fields[64];
        int nested_count = 0;
        asun_err_t err = asun_parse_schema(pos, end, nested_fields, &nested_count, 64);
        if (err != ASUN_OK) return err;
        asun_free_schema_fields(nested_fields, nested_count);
        return ASUN_OK;
    }
    if (**pos == '[') {
        (*pos)++;
        asun_skip_ws(pos, end);
        if (*pos < end && **pos == ']') { (*pos)++; return ASUN_OK; }
        if (*pos < end && **pos == '{') {
            asun_schema_field_t nested_fields[64];
            int nested_count = 0;
            asun_err_t err = asun_parse_schema(pos, end, nested_fields, &nested_count, 64);
            if (err != ASUN_OK) return err;
            asun_free_schema_fields(nested_fields, nested_count);
        } else {
            asun_err_t err = asun_validate_schema_scalar_type(pos, end);
            if (err != ASUN_OK) return err;
        }
        asun_skip_ws(pos, end);
        if (*pos >= end || **pos != ']') return ASUN_ERR_SYNTAX;
        (*pos)++;
        return ASUN_OK;
    }
    return asun_validate_schema_scalar_type(pos, end);
}

static asun_err_t asun_parse_schema(const char** pos, const char* end,
                                    asun_schema_field_t* fields, int* count, int max_fields) {
    asun_skip_ws(pos, end);
    if (*pos >= end || **pos != '{') return ASUN_ERR_SYNTAX;
    (*pos)++;
    int n = 0;
    while (n < max_fields) {
        asun_skip_ws(pos, end);
        if (*pos >= end) return ASUN_ERR_SYNTAX;
        if (**pos == '}') { (*pos)++; break; }
        if (n > 0) {
            if (**pos == ',') (*pos)++;
            else return ASUN_ERR_SYNTAX;
            asun_skip_ws(pos, end);
        }
        /* Field name */
        if (**pos == '"') {
            const char* before = *pos;
            char* decoded = NULL;
            size_t name_len = 0;
            asun_err_t err = asun_parse_quoted_string(pos, end, &decoded, &name_len);
            if (err != ASUN_OK) return err;
            fields[n].name = decoded;
            fields[n].len = name_len;
            fields[n].allocated = (decoded < before || decoded >= *pos);
        } else {
            const char* name_start = *pos;
            while (*pos < end && **pos != ',' && **pos != '@' && **pos != '}' &&
                   **pos != ' ' && **pos != '\t' && **pos != '\n' && **pos != '\r' &&
                   **pos != '{') (*pos)++;
            size_t name_len = *pos - name_start;
            fields[n].name = name_start;
            fields[n].len = name_len;
            fields[n].allocated = false;
        }
        n++;
        /* Validate and skip type annotation if present: @type  or  @{...} or @[...] */
        asun_skip_ws(pos, end);
        if (*pos < end && **pos == '@') {
            (*pos)++;
            asun_skip_ws(pos, end);
            asun_err_t err = asun_validate_schema_annotation(pos, end);
            if (err != ASUN_OK) return err;
        }
    }
    *count = n;
    return ASUN_OK;
}

static void asun_free_schema_fields(asun_schema_field_t* fields, int count) {
    for (int i = 0; i < count; i++) {
        if (fields[i].allocated && fields[i].name) free((void*)fields[i].name);
    }
}

/* ============================================================================
 * Generic field dump/load functions
 * ============================================================================ */

void asun_encode_bool(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_i8(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_i16(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_i32(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_i64(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_u8(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_u16(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_u32(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_u64(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_f32(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_f64(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_char(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_str(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_opt_i64(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_opt_str(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_opt_f64(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_vec_i64(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_vec_u64(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_vec_f64(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_vec_str(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_vec_bool(asun_buf_t* buf, const void* base, size_t offset);
void asun_encode_vec_vec_i64(asun_buf_t* buf, const void* base, size_t offset);

asun_err_t asun_decode_bool(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_i8(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_i16(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_i32(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_i64(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_u8(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_u16(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_u32(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_u64(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_f32(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_f64(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_char(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_str(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_opt_i64(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_opt_str(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_opt_f64(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_vec_i64(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_vec_u64(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_vec_f64(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_vec_str(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_vec_bool(const char** pos, const char* end, void* base, size_t offset);
asun_err_t asun_decode_vec_vec_i64(const char** pos, const char* end, void* base, size_t offset);

/* Generic struct dump/load via descriptor */
void asun_encode_struct(asun_buf_t* buf, const void* obj, const asun_desc_t* desc);
asun_err_t asun_decode_struct(const char** pos, const char* end, void* obj, const asun_desc_t* desc);

/* Recursive schema writers */
void asun_write_schema(asun_buf_t* buf, const asun_desc_t* desc);
void asun_write_schema_typed(asun_buf_t* buf, const asun_desc_t* desc);

/* Pretty-format: reformat compact ASUN with smart indentation */
asun_buf_t asun_pretty_format(const char* src, size_t len);

/* ============================================================================
 * ASUN_FIELD macro — build a field descriptor
 * ============================================================================ */

#define ASUN_DUMP_FN(t) asun_encode_##t
#define ASUN_LOAD_FN(t) asun_decode_##t

/* C _Bool aliases: bool expands to _Bool before ## token pasting */
#define asun_encode__Bool asun_encode_bool
#define asun_decode__Bool asun_decode_bool
#define ASUN_TYPE_NAME__Bool "bool"
#define ASUN_TYPE_ENUM__Bool ASUN_BOOL

#define ASUN_TYPE_NAME_bool  "bool"
#define ASUN_TYPE_NAME_i8    "int"
#define ASUN_TYPE_NAME_i16   "int"
#define ASUN_TYPE_NAME_i32   "int"
#define ASUN_TYPE_NAME_i64   "int"
#define ASUN_TYPE_NAME_u8    "int"
#define ASUN_TYPE_NAME_u16   "int"
#define ASUN_TYPE_NAME_u32   "int"
#define ASUN_TYPE_NAME_u64   "int"
#define ASUN_TYPE_NAME_f32   "float"
#define ASUN_TYPE_NAME_f64   "float"
#define ASUN_TYPE_NAME_char  "char"
#define ASUN_TYPE_NAME_str   "str"
#define ASUN_TYPE_NAME_opt_i64 "int"
#define ASUN_TYPE_NAME_opt_str "str"
#define ASUN_TYPE_NAME_opt_f64 "float"
#define ASUN_TYPE_NAME_vec_i64 "[int]"
#define ASUN_TYPE_NAME_vec_u64 "[int]"
#define ASUN_TYPE_NAME_vec_f64 "[float]"
#define ASUN_TYPE_NAME_vec_str "[str]"
#define ASUN_TYPE_NAME_vec_bool "[bool]"
#define ASUN_TYPE_NAME_vec_vec_i64 "[[int]]"

#define ASUN_TYPE_ENUM_bool  ASUN_BOOL
#define ASUN_TYPE_ENUM_i8    ASUN_I8
#define ASUN_TYPE_ENUM_i16   ASUN_I16
#define ASUN_TYPE_ENUM_i32   ASUN_I32
#define ASUN_TYPE_ENUM_i64   ASUN_I64
#define ASUN_TYPE_ENUM_u8    ASUN_U8
#define ASUN_TYPE_ENUM_u16   ASUN_U16
#define ASUN_TYPE_ENUM_u32   ASUN_U32
#define ASUN_TYPE_ENUM_u64   ASUN_U64
#define ASUN_TYPE_ENUM_f32   ASUN_F32
#define ASUN_TYPE_ENUM_f64   ASUN_F64
#define ASUN_TYPE_ENUM_char  ASUN_CHAR
#define ASUN_TYPE_ENUM_str   ASUN_STR
#define ASUN_TYPE_ENUM_opt_i64 ASUN_OPT_I64
#define ASUN_TYPE_ENUM_opt_str ASUN_OPT_STR
#define ASUN_TYPE_ENUM_opt_f64 ASUN_OPT_F64
#define ASUN_TYPE_ENUM_vec_i64 ASUN_VEC_I64
#define ASUN_TYPE_ENUM_vec_u64 ASUN_VEC_U64
#define ASUN_TYPE_ENUM_vec_f64 ASUN_VEC_F64
#define ASUN_TYPE_ENUM_vec_str ASUN_VEC_STR
#define ASUN_TYPE_ENUM_vec_bool ASUN_VEC_BOOL
#define ASUN_TYPE_ENUM_vec_vec_i64 ASUN_VEC_VEC_I64

#define ASUN_FIELD(StructType, member, fname, ftype) \
    { fname, sizeof(fname) - 1, ASUN_TYPE_ENUM_##ftype, \
      offsetof(StructType, member), ASUN_TYPE_NAME_##ftype, NULL, \
      ASUN_DUMP_FN(ftype), ASUN_LOAD_FN(ftype) }

#define ASUN_FIELD_STRUCT(StructType, member, fname, sub_desc_ptr) \
    { fname, sizeof(fname) - 1, ASUN_STRUCT, \
      offsetof(StructType, member), NULL, sub_desc_ptr, \
      NULL, NULL }

/* ============================================================================
 * ASUN_FIELDS — auto-generates descriptor, dump, and load functions
 * ============================================================================ */

#define ASUN_FIELDS(StructType, nfields, ...) \
    static const asun_field_t StructType##_asun_fields[] = { __VA_ARGS__ }; \
    static const asun_desc_t StructType##_asun_desc = { \
        #StructType, StructType##_asun_fields, nfields \
    }; \
    static inline asun_buf_t asun_encode_##StructType(const StructType* obj) { \
        asun_buf_t buf = asun_buf_new(128); \
        asun_write_schema(&buf, &StructType##_asun_desc); \
        asun_buf_push(&buf, ':'); \
        asun_buf_push(&buf, '('); \
        for (int i = 0; i < nfields; i++) { \
            if (i > 0) asun_buf_push(&buf, ','); \
            if (StructType##_asun_fields[i].type == ASUN_STRUCT) { \
                if (StructType##_asun_fields[i].dump_fn) { \
                    StructType##_asun_fields[i].dump_fn(&buf, obj, \
                        StructType##_asun_fields[i].offset); \
                } else { \
                    asun_encode_struct(&buf, (const char*)obj + StructType##_asun_fields[i].offset, \
                                     (const asun_desc_t*)StructType##_asun_fields[i].sub_desc); \
                } \
            } else { \
                StructType##_asun_fields[i].dump_fn(&buf, obj, \
                    StructType##_asun_fields[i].offset); \
            } \
        } \
        asun_buf_push(&buf, ')'); \
        return buf; \
    } \
    static inline asun_buf_t asun_encode_typed_##StructType(const StructType* obj) { \
        asun_buf_t buf = asun_buf_new(128); \
        asun_write_schema_typed(&buf, &StructType##_asun_desc); \
        asun_buf_push(&buf, ':'); \
        asun_buf_push(&buf, '('); \
        for (int i = 0; i < nfields; i++) { \
            if (i > 0) asun_buf_push(&buf, ','); \
            if (StructType##_asun_fields[i].type == ASUN_STRUCT) { \
                if (StructType##_asun_fields[i].dump_fn) { \
                    StructType##_asun_fields[i].dump_fn(&buf, obj, \
                        StructType##_asun_fields[i].offset); \
                } else { \
                    asun_encode_struct(&buf, (const char*)obj + StructType##_asun_fields[i].offset, \
                                     (const asun_desc_t*)StructType##_asun_fields[i].sub_desc); \
                } \
            } else { \
                StructType##_asun_fields[i].dump_fn(&buf, obj, \
                    StructType##_asun_fields[i].offset); \
            } \
        } \
        asun_buf_push(&buf, ')'); \
        return buf; \
    } \
    static inline asun_err_t asun_decode_##StructType(const char* input, size_t len, StructType* out) { \
        const char* pos = input; \
        const char* end = input + len; \
        asun_skip_ws(&pos, end); \
        if (pos >= end || *pos != '{') return ASUN_ERR_SYNTAX; \
        asun_schema_field_t schema[64]; \
        int schema_count = 0; \
        asun_err_t err = asun_parse_schema(&pos, end, schema, &schema_count, 64); \
        if (err != ASUN_OK) return err; \
        asun_skip_ws(&pos, end); \
        if (pos >= end || *pos != ':') { asun_free_schema_fields(schema, schema_count); return ASUN_ERR_SYNTAX; } \
        pos++; \
        asun_skip_ws(&pos, end); \
        if (pos >= end || *pos != '(') { asun_free_schema_fields(schema, schema_count); return ASUN_ERR_SYNTAX; } \
        pos++; \
        int field_map[64]; \
        for (int i = 0; i < schema_count; i++) { \
            field_map[i] = -1; \
            for (int j = 0; j < nfields; j++) { \
                if (schema[i].len == StructType##_asun_fields[j].name_len && \
                    memcmp(schema[i].name, StructType##_asun_fields[j].name, schema[i].len) == 0) { \
                    field_map[i] = j; break; \
                } \
            } \
        } \
        for (int i = 0; i < schema_count; i++) { \
            asun_skip_ws(&pos, end); \
            if (pos < end && *pos == ')') break; \
            if (i > 0) { \
                if (*pos == ',') { pos++; asun_skip_ws(&pos, end); if (pos < end && *pos == ')') break; } \
                else if (*pos == ')') break; \
                else { asun_free_schema_fields(schema, schema_count); return ASUN_ERR_SYNTAX; } \
            } \
            if (field_map[i] >= 0) { \
                int fi = field_map[i]; \
                if (StructType##_asun_fields[fi].type == ASUN_STRUCT) { \
                    if (StructType##_asun_fields[fi].load_fn) { \
                        err = StructType##_asun_fields[fi].load_fn(&pos, end, out, \
                            StructType##_asun_fields[fi].offset); \
                    } else { \
                        err = asun_decode_struct(&pos, end, \
                            (char*)out + StructType##_asun_fields[fi].offset, \
                            (const asun_desc_t*)StructType##_asun_fields[fi].sub_desc); \
                    } \
                } else { \
                    err = StructType##_asun_fields[fi].load_fn(&pos, end, out, \
                        StructType##_asun_fields[fi].offset); \
                } \
                if (err != ASUN_OK) { asun_free_schema_fields(schema, schema_count); return err; } \
            } else { \
                asun_skip_value(&pos, end); \
            } \
        } \
        asun_skip_ws(&pos, end); \
        if (pos < end && *pos == ')') pos++; \
        asun_skip_ws(&pos, end); \
        if (pos < end) { asun_free_schema_fields(schema, schema_count); return ASUN_ERR_SYNTAX; } \
        asun_free_schema_fields(schema, schema_count); \
        return ASUN_OK; \
    } \
    /* encode_typed_vec: [{field@type,...}]:(row1),(row2),... */ \
    static inline asun_buf_t asun_encode_typed_vec_##StructType(const StructType* arr, size_t count) { \
        asun_buf_t buf = asun_buf_new(count * 64 + 128); \
        asun_buf_push(&buf, '['); \
        asun_write_schema_typed(&buf, &StructType##_asun_desc); \
        asun_buf_push(&buf, ']'); \
        asun_buf_push(&buf, ':'); \
        for (size_t r = 0; r < count; r++) { \
            if (r > 0) asun_buf_push(&buf, ','); \
            asun_buf_push(&buf, '('); \
            for (int i = 0; i < nfields; i++) { \
                if (i > 0) asun_buf_push(&buf, ','); \
                if (StructType##_asun_fields[i].type == ASUN_STRUCT) { \
                    if (StructType##_asun_fields[i].dump_fn) { \
                        StructType##_asun_fields[i].dump_fn(&buf, &arr[r], \
                            StructType##_asun_fields[i].offset); \
                    } else { \
                        asun_encode_struct(&buf, (const char*)&arr[r] + StructType##_asun_fields[i].offset, \
                                         (const asun_desc_t*)StructType##_asun_fields[i].sub_desc); \
                    } \
                } else { \
                    StructType##_asun_fields[i].dump_fn(&buf, &arr[r], \
                        StructType##_asun_fields[i].offset); \
                } \
            } \
            asun_buf_push(&buf, ')'); \
        } \
        return buf; \
    } \
    /* encode_vec: [{schema}]:(row1),(row2),... */ \
    static inline asun_buf_t asun_encode_vec_##StructType(const StructType* arr, size_t count) { \
        asun_buf_t buf = asun_buf_new(count * 64 + 128); \
        asun_buf_push(&buf, '['); \
        asun_write_schema(&buf, &StructType##_asun_desc); \
        asun_buf_push(&buf, ']'); \
        asun_buf_push(&buf, ':'); \
        for (size_t r = 0; r < count; r++) { \
            if (r > 0) asun_buf_push(&buf, ','); \
            asun_buf_push(&buf, '('); \
            for (int i = 0; i < nfields; i++) { \
                if (i > 0) asun_buf_push(&buf, ','); \
                if (StructType##_asun_fields[i].type == ASUN_STRUCT) { \
                    if (StructType##_asun_fields[i].dump_fn) { \
                        StructType##_asun_fields[i].dump_fn(&buf, &arr[r], \
                            StructType##_asun_fields[i].offset); \
                    } else { \
                        asun_encode_struct(&buf, (const char*)&arr[r] + StructType##_asun_fields[i].offset, \
                                         (const asun_desc_t*)StructType##_asun_fields[i].sub_desc); \
                    } \
                } else { \
                    StructType##_asun_fields[i].dump_fn(&buf, &arr[r], \
                        StructType##_asun_fields[i].offset); \
                } \
            } \
            asun_buf_push(&buf, ')'); \
        } \
        return buf; \
    } \
    /* encode_pretty: pretty-formatted single struct */ \
    static inline asun_buf_t asun_encode_pretty_##StructType(const StructType* obj) { \
        asun_buf_t compact = asun_encode_##StructType(obj); \
        asun_buf_t pretty = asun_pretty_format(compact.data, compact.len); \
        asun_buf_free(&compact); \
        return pretty; \
    } \
    /* encode_pretty_typed: pretty-formatted single struct with types */ \
    static inline asun_buf_t asun_encode_pretty_typed_##StructType(const StructType* obj) { \
        asun_buf_t compact = asun_encode_typed_##StructType(obj); \
        asun_buf_t pretty = asun_pretty_format(compact.data, compact.len); \
        asun_buf_free(&compact); \
        return pretty; \
    } \
    /* encode_pretty_vec: pretty-formatted array */ \
    static inline asun_buf_t asun_encode_pretty_vec_##StructType(const StructType* arr, size_t count) { \
        asun_buf_t compact = asun_encode_vec_##StructType(arr, count); \
        asun_buf_t pretty = asun_pretty_format(compact.data, compact.len); \
        asun_buf_free(&compact); \
        return pretty; \
    } \
    /* encode_pretty_typed_vec: pretty-formatted typed array */ \
    static inline asun_buf_t asun_encode_pretty_typed_vec_##StructType(const StructType* arr, size_t count) { \
        asun_buf_t compact = asun_encode_typed_vec_##StructType(arr, count); \
        asun_buf_t pretty = asun_pretty_format(compact.data, compact.len); \
        asun_buf_free(&compact); \
        return pretty; \
    } \
    /* decode_vec: [{schema}]:(row1),(row2),... */ \
    static inline asun_err_t asun_decode_vec_##StructType(const char* input, size_t len, \
                                                         StructType** out, size_t* out_count) { \
        const char* pos = input; \
        const char* end = input + len; \
        asun_skip_ws(&pos, end); \
        if (pos >= end || *pos != '[') return ASUN_ERR_SYNTAX; \
        pos++; \
        if (pos >= end || *pos != '{') return ASUN_ERR_SYNTAX; \
        asun_schema_field_t schema[64]; \
        int schema_count = 0; \
        asun_err_t err = asun_parse_schema(&pos, end, schema, &schema_count, 64); \
        if (err != ASUN_OK) return err; \
        asun_skip_ws(&pos, end); \
        if (pos >= end || *pos != ']') { asun_free_schema_fields(schema, schema_count); return ASUN_ERR_SYNTAX; } \
        pos++; \
        asun_skip_ws(&pos, end); \
        if (pos >= end || *pos != ':') { asun_free_schema_fields(schema, schema_count); return ASUN_ERR_SYNTAX; } \
        pos++; \
        int field_map[64]; \
        for (int i = 0; i < schema_count; i++) { \
            field_map[i] = -1; \
            for (int j = 0; j < nfields; j++) { \
                if (schema[i].len == StructType##_asun_fields[j].name_len && \
                    memcmp(schema[i].name, StructType##_asun_fields[j].name, schema[i].len) == 0) { \
                    field_map[i] = j; break; \
                } \
            } \
        } \
        size_t cap = 16; \
        size_t cnt = 0; \
        StructType* arr = (StructType*)calloc(cap, sizeof(StructType)); \
        while (1) { \
            asun_skip_ws(&pos, end); \
            if (pos >= end || *pos != '(') break; \
            pos++; \
            if (cnt >= cap) { \
                cap = cap + (cap >> 1); \
                arr = (StructType*)realloc(arr, cap * sizeof(StructType)); \
                memset(arr + cnt, 0, (cap - cnt) * sizeof(StructType)); \
            } \
            StructType* elem = &arr[cnt]; \
            memset(elem, 0, sizeof(StructType)); \
            for (int i = 0; i < schema_count; i++) { \
                asun_skip_ws(&pos, end); \
                if (pos < end && *pos == ')') break; \
                if (i > 0) { \
                    if (*pos == ',') { pos++; asun_skip_ws(&pos, end); if (pos < end && *pos == ')') break; } \
                    else if (*pos == ')') break; \
                    else { asun_free_schema_fields(schema, schema_count); free(arr); return ASUN_ERR_SYNTAX; } \
                } \
                if (field_map[i] >= 0) { \
                    int fi = field_map[i]; \
                    if (StructType##_asun_fields[fi].type == ASUN_STRUCT) { \
                        if (StructType##_asun_fields[fi].load_fn) { \
                            err = StructType##_asun_fields[fi].load_fn(&pos, end, elem, \
                                StructType##_asun_fields[fi].offset); \
                        } else { \
                            err = asun_decode_struct(&pos, end, \
                                (char*)elem + StructType##_asun_fields[fi].offset, \
                                (const asun_desc_t*)StructType##_asun_fields[fi].sub_desc); \
                        } \
                    } else { \
                        err = StructType##_asun_fields[fi].load_fn(&pos, end, elem, \
                            StructType##_asun_fields[fi].offset); \
                    } \
                    if (err != ASUN_OK) { asun_free_schema_fields(schema, schema_count); free(arr); return err; } \
                } else { \
                    asun_skip_value(&pos, end); \
                } \
            } \
            asun_skip_remaining_tuple_values(&pos, end); \
            asun_skip_ws(&pos, end); \
            if (pos < end && *pos == ')') pos++; \
            cnt++; \
            asun_skip_ws(&pos, end); \
            if (pos < end && *pos == ',') { \
                pos++; \
                asun_skip_ws(&pos, end); \
                if (pos >= end || *pos != '(') break; \
            } \
        } \
        asun_free_schema_fields(schema, schema_count); \
        *out = arr; \
        *out_count = cnt; \
        return ASUN_OK; \
    }

/* ============================================================================
 * Struct field as dump_fn for vector-of-struct fields
 * ============================================================================ */

/* For struct-typed vector fields, we need a vec type per struct.
 * Use ASUN_VEC_STRUCT_DEFINE to create the necessary types. */

#define ASUN_VEC_STRUCT_DEFINE(StructType) \
    ASUN_VEC_DEFINE(asun_vec_##StructType, StructType) \
    static inline void asun_encode_vec_struct_##StructType(asun_buf_t* buf, const void* base, size_t offset) { \
        const asun_vec_##StructType* v = (const asun_vec_##StructType*)((const char*)base + offset); \
        asun_buf_push(buf, '['); \
        for (size_t i = 0; i < v->len; i++) { \
            if (i > 0) asun_buf_push(buf, ','); \
            asun_encode_struct(buf, &v->data[i], &StructType##_asun_desc); \
        } \
        asun_buf_push(buf, ']'); \
    } \
    static inline asun_err_t asun_decode_vec_struct_##StructType(const char** pos, const char* end, void* base, size_t offset) { \
        asun_skip_ws(pos, end); \
        if (*pos >= end || **pos != '[') return ASUN_ERR_SYNTAX; \
        (*pos)++; \
        asun_vec_##StructType* v = (asun_vec_##StructType*)((char*)base + offset); \
        *v = asun_vec_##StructType##_new(); \
        bool first = true; \
        while (1) { \
            asun_skip_ws(pos, end); \
            if (*pos >= end || **pos == ']') { (*pos)++; break; } \
            if (!first) { \
                if (**pos == ',') { (*pos)++; asun_skip_ws(pos, end); if (*pos < end && **pos == ']') { (*pos)++; break; } } \
                else break; \
            } \
            first = false; \
            /* Ensure capacity and load directly into vector slot */ \
            if (v->len >= v->cap) { \
                v->cap = v->cap ? v->cap * 2 : 4; \
                v->data = (StructType*)realloc(v->data, v->cap * sizeof(StructType)); \
            } \
            memset(&v->data[v->len], 0, sizeof(StructType)); \
            asun_err_t err = asun_decode_struct(pos, end, &v->data[v->len], &StructType##_asun_desc); \
            if (err != ASUN_OK) return err; \
            v->len++; \
        } \
        return ASUN_OK; \
    }

/* Field macro for vec-of-struct */
#define ASUN_FIELD_VEC_STRUCT(StructType, member, fname, ElemType) \
    { fname, sizeof(fname) - 1, ASUN_STRUCT, \
      offsetof(StructType, member), NULL, &ElemType##_asun_desc, \
      (asun_encode_fn)asun_encode_vec_struct_##ElemType, \
      (asun_decode_fn)asun_decode_vec_struct_##ElemType }

/* ============================================================================
 * Binary serialization / deserialization (ASUN-BIN)
 * Little-endian fixed-width encoding, zero-copy string_view for reads.
 * ============================================================================
 */

/* Binary write helpers — append into asun_buf_t */
asun_inline void asun_bin_write_u8(asun_buf_t* buf, uint8_t v) {
    asun_buf_push(buf, (char)v);
}
asun_inline void asun_bin_write_u16(asun_buf_t* buf, uint16_t v) {
    uint8_t tmp[2]; memcpy(tmp, &v, 2); asun_buf_append(buf, (char*)tmp, 2);
}
asun_inline void asun_bin_write_u32(asun_buf_t* buf, uint32_t v) {
    uint8_t tmp[4]; memcpy(tmp, &v, 4); asun_buf_append(buf, (char*)tmp, 4);
}
asun_inline void asun_bin_write_u64(asun_buf_t* buf, uint64_t v) {
    uint8_t tmp[8]; memcpy(tmp, &v, 8); asun_buf_append(buf, (char*)tmp, 8);
}
asun_inline void asun_bin_write_f32(asun_buf_t* buf, float v) {
    uint32_t u; memcpy(&u, &v, 4); asun_bin_write_u32(buf, u);
}
asun_inline void asun_bin_write_f64(asun_buf_t* buf, double v) {
    uint64_t u; memcpy(&u, &v, 8); asun_bin_write_u64(buf, u);
}
asun_inline void asun_bin_write_str(asun_buf_t* buf, const char* s, size_t len) {
    asun_bin_write_u32(buf, (uint32_t)len);
    asun_buf_append(buf, s, len);
}
asun_inline void asun_bin_write_asun_string(asun_buf_t* buf, const asun_string_t* s) {
    if (s && s->data) {
        asun_bin_write_str(buf, s->data, s->len);
    } else {
        asun_bin_write_u32(buf, 0);
    }
}

/* Binary read helpers — reads from cursor, advances pos, zero-alloc for strings */
asun_inline asun_err_t asun_bin_read_u8(const char** pos, const char* end, uint8_t* out) {
    if (*pos + 1 > end) return ASUN_ERR_BUFFER_OVERFLOW;
    *out = (uint8_t)**pos; (*pos)++;
    return ASUN_OK;
}
asun_inline asun_err_t asun_bin_read_u16(const char** pos, const char* end, uint16_t* out) {
    if (*pos + 2 > end) return ASUN_ERR_BUFFER_OVERFLOW;
    memcpy(out, *pos, 2); (*pos) += 2;
    return ASUN_OK;
}
asun_inline asun_err_t asun_bin_read_u32(const char** pos, const char* end, uint32_t* out) {
    if (*pos + 4 > end) return ASUN_ERR_BUFFER_OVERFLOW;
    memcpy(out, *pos, 4); (*pos) += 4;
    return ASUN_OK;
}
asun_inline asun_err_t asun_bin_read_u64(const char** pos, const char* end, uint64_t* out) {
    if (*pos + 8 > end) return ASUN_ERR_BUFFER_OVERFLOW;
    memcpy(out, *pos, 8); (*pos) += 8;
    return ASUN_OK;
}
asun_inline asun_err_t asun_bin_read_f32(const char** pos, const char* end, float* out) {
    uint32_t u;
    if (*pos + 4 > end) return ASUN_ERR_BUFFER_OVERFLOW;
    memcpy(&u, *pos, 4); (*pos) += 4;
    memcpy(out, &u, 4);
    return ASUN_OK;
}
asun_inline asun_err_t asun_bin_read_f64(const char** pos, const char* end, double* out) {
    uint64_t u;
    if (*pos + 8 > end) return ASUN_ERR_BUFFER_OVERFLOW;
    memcpy(&u, *pos, 8); (*pos) += 8;
    memcpy(out, &u, 8);
    return ASUN_OK;
}
/* Zero-copy: points directly into the source buffer — caller must keep source alive */
asun_inline asun_err_t asun_bin_read_str_view(const char** pos, const char* end,
                                               const char** out_data, size_t* out_len) {
    uint32_t len;
    if (*pos + 4 > end) return ASUN_ERR_BUFFER_OVERFLOW;
    memcpy(&len, *pos, 4); (*pos) += 4;
    if (*pos + len > end) return ASUN_ERR_BUFFER_OVERFLOW;
    *out_data = *pos;
    *out_len = len;
    (*pos) += len;
    return ASUN_OK;
}
/* Heap-allocating variant — caller must free */
asun_inline asun_err_t asun_bin_read_str_alloc(const char** pos, const char* end,
                                                char** out) {
    const char* d; size_t l;
    asun_err_t e = asun_bin_read_str_view(pos, end, &d, &l);
    if (e != ASUN_OK) return e;
    char* s = (char*)malloc(l + 1);
    if (!s) return ASUN_ERR_ALLOC;
    memcpy(s, d, l); s[l] = '\0';
    *out = s;
    return ASUN_OK;
}
/* Read into asun_string_t — zero-copy (points into source buffer) */
asun_inline asun_err_t asun_bin_read_asun_string(const char** pos, const char* end,
                                                  asun_string_t* out) {
    const char* d; size_t l;
    asun_err_t e = asun_bin_read_str_view(pos, end, &d, &l);
    if (e != ASUN_OK) return e;
    out->data = (char*)(uintptr_t)d;   /* cast away const: zero-copy, caller keeps source alive */
    out->len = l;
    return ASUN_OK;
}

/* Type-specific binary dump field helpers — same signature as asun_encode_fn */
void asun_bin_encode_bool  (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_i8    (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_i16   (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_i32   (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_i64   (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_u8    (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_u16   (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_u32   (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_u64   (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_f32   (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_f64   (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_str   (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_vec_i64  (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_vec_u64  (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_vec_f64  (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_vec_str  (asun_buf_t* buf, const void* base, size_t off);
void asun_bin_encode_vec_bool (asun_buf_t* buf, const void* base, size_t off);

asun_err_t asun_bin_decode_bool  (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_i8    (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_i16   (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_i32   (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_i64   (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_u8    (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_u16   (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_u32   (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_u64   (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_f32   (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_f64   (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_str   (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_vec_i64  (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_vec_u64  (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_vec_f64  (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_vec_str  (const char** pos, const char* end, void* base, size_t off);
asun_err_t asun_bin_decode_vec_bool (const char** pos, const char* end, void* base, size_t off);

/* Generic struct binary dump/load using the existing descriptor */
void       asun_bin_encode_struct(asun_buf_t* buf, const void* obj, const asun_desc_t* desc);
asun_err_t asun_bin_decode_struct(const char** pos, const char* end, void* obj, const asun_desc_t* desc);

/* Convenience binary-fn selectors (mirrors ASUN_DUMP_FN / ASUN_LOAD_FN) */
#define ASUN_BIN_DUMP_FN(t) asun_bin_encode_##t
#define ASUN_BIN_LOAD_FN(t) asun_bin_decode_##t
#define asun_bin_encode__Bool asun_bin_encode_bool
#define asun_bin_decode__Bool asun_bin_decode_bool

/* ASUN_FIELDS_BIN adds binary dump/load to an already-declared ASUN_FIELDS struct.
 * Use after ASUN_FIELDS to inject asun_encode_bin_<T>, asun_decode_bin_<T>, etc. */
#define ASUN_FIELDS_BIN(StructType, nfields) \
    static inline asun_buf_t asun_encode_bin_##StructType(const StructType* obj) { \
        asun_buf_t buf = asun_buf_new(nfields * 16); \
        asun_bin_encode_struct(&buf, obj, &StructType##_asun_desc); \
        return buf; \
    } \
    static inline asun_buf_t asun_encode_bin_vec_##StructType(const StructType* arr, size_t count) { \
        asun_buf_t buf = asun_buf_new(count * nfields * 16 + 8); \
        uint32_t n = (uint32_t)count; \
        asun_bin_write_u32(&buf, n); \
        for (size_t i = 0; i < count; i++) { \
            asun_bin_encode_struct(&buf, &arr[i], &StructType##_asun_desc); \
        } \
        return buf; \
    } \
    static inline asun_err_t asun_decode_bin_##StructType(const char* data, size_t len, StructType* out) { \
        const char* pos = data; \
        const char* end = data + len; \
        return asun_bin_decode_struct(&pos, end, out, &StructType##_asun_desc); \
    } \
    static inline asun_err_t asun_decode_bin_vec_##StructType(const char* data, size_t len, \
                                                              StructType** out_arr, size_t* out_count) { \
        const char* pos = data; \
        const char* end = data + len; \
        uint32_t count; \
        asun_err_t err = asun_bin_read_u32(&pos, end, &count); \
        if (err != ASUN_OK) return err; \
        StructType* arr = (StructType*)calloc(count, sizeof(StructType)); \
        if (!arr) return ASUN_ERR_ALLOC; \
        for (uint32_t i = 0; i < count; i++) { \
            err = asun_bin_decode_struct(&pos, end, &arr[i], &StructType##_asun_desc); \
            if (err != ASUN_OK) { free(arr); return err; } \
        } \
        *out_arr = arr; \
        *out_count = count; \
        return ASUN_OK; \
    }

#ifdef __cplusplus
}
#endif

#endif /* ASUN_H */
