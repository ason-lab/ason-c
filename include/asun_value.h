/*
 * ASUN — Untyped Value support (C, depends on asun.h)
 *
 * Provides a generic Value tree (null / bool / int / double / string / array)
 * for cases where the schema is unknown at compile time, used for conformance
 * testing and schema-less consumers. Re-uses SIMD scanning, escape handling,
 * and fast itoa/dtoa from <asun.h> so the typed code paths are not regressed.
 *
 *   asun_value_t* v = asun_value_decode(input, len);
 *   asun_buf_t out  = asun_value_encode(v);
 *   asun_value_free(v);
 *
 * Performance notes:
 *   * Tagged union (`asun_value_t`) — single load + branch for every dispatch.
 *   * Plain-token scanning is the same byte-at-a-time loop used by the typed
 *     decoder; for short tokens (the common case) this is faster than calling
 *     a SIMD helper.
 *   * Encoding goes through `asun_buf_append_str` (SIMD quoting check) and
 *     the existing `asun_buf_append_i64` / `asun_buf_append_f64`.
 */

#ifndef ASUN_VALUE_H
#define ASUN_VALUE_H

#include "asun.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ASUN_VAL_NULL = 0,
    ASUN_VAL_BOOL,
    ASUN_VAL_INT,
    ASUN_VAL_DOUBLE,
    ASUN_VAL_STRING,
    ASUN_VAL_ARRAY,
} asun_value_tag_t;

typedef struct asun_value_s asun_value_t;

struct asun_value_s {
    asun_value_tag_t tag;
    /* scalar payload — only one is meaningful per tag */
    bool          b;
    int64_t       i;
    double        d;
    /* heap payload */
    char*         s;       /* String: NUL-terminated, owned */
    size_t        s_len;
    asun_value_t* arr;     /* Array: owned, length=arr_len */
    size_t        arr_len;
    size_t        arr_cap;
};

/* ---- constructors / lifecycle ------------------------------------------- */

static inline asun_value_t* asun_value_alloc(asun_value_tag_t tag) {
    asun_value_t* v = (asun_value_t*)calloc(1, sizeof(asun_value_t));
    v->tag = tag;
    return v;
}

static void asun_value_clear(asun_value_t* v); /* fwd */

static inline void asun_value_free(asun_value_t* v) {
    if (!v) return;
    asun_value_clear(v);
    free(v);
}

static void asun_value_clear(asun_value_t* v) {
    if (!v) return;
    if (v->tag == ASUN_VAL_STRING && v->s) { free(v->s); v->s = NULL; v->s_len = 0; }
    if (v->tag == ASUN_VAL_ARRAY && v->arr) {
        for (size_t k = 0; k < v->arr_len; k++) asun_value_clear(&v->arr[k]);
        free(v->arr);
        v->arr = NULL;
        v->arr_len = v->arr_cap = 0;
    }
    v->tag = ASUN_VAL_NULL;
    v->b = false; v->i = 0; v->d = 0.0;
}

static inline asun_value_t* asun_value_make_null(void)        { return asun_value_alloc(ASUN_VAL_NULL); }
static inline asun_value_t* asun_value_make_bool(bool x)      { asun_value_t* v = asun_value_alloc(ASUN_VAL_BOOL); v->b = x; return v; }
static inline asun_value_t* asun_value_make_int(int64_t x)    { asun_value_t* v = asun_value_alloc(ASUN_VAL_INT); v->i = x; return v; }
static inline asun_value_t* asun_value_make_double(double x)  { asun_value_t* v = asun_value_alloc(ASUN_VAL_DOUBLE); v->d = x; return v; }

static inline asun_value_t* asun_value_make_string(const char* s, size_t len) {
    asun_value_t* v = asun_value_alloc(ASUN_VAL_STRING);
    v->s = (char*)malloc(len + 1);
    if (len) memcpy(v->s, s, len);
    v->s[len] = '\0';
    v->s_len = len;
    return v;
}

static inline void asun_value_array_push(asun_value_t* v, asun_value_t* child) {
    if (v->arr_len >= v->arr_cap) {
        size_t nc = v->arr_cap ? v->arr_cap + (v->arr_cap >> 1) : 4;
        v->arr = (asun_value_t*)realloc(v->arr, nc * sizeof(asun_value_t));
        v->arr_cap = nc;
    }
    /* move-by-value; clear the source pointer so caller doesn't double-free */
    v->arr[v->arr_len++] = *child;
    free(child); /* free shell; payload now owned by v->arr[k] */
}

/* ---- equality (numeric int/double cross-compare matches conformance harness) -- */

static inline bool asun_double_eq(double a, double b) {
    if (a == b) return true;
    double diff = a > b ? a - b : b - a;
    double aa = a < 0 ? -a : a;
    double bb = b < 0 ? -b : b;
    double scale = aa > bb ? aa : bb;
    double tol = scale * 1e-12;
    if (tol < 1e-12) tol = 1e-12;
    return diff <= tol;
}

static inline bool asun_value_eq(const asun_value_t* a, const asun_value_t* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->tag == ASUN_VAL_INT && b->tag == ASUN_VAL_DOUBLE) return asun_double_eq((double)a->i, b->d);
    if (a->tag == ASUN_VAL_DOUBLE && b->tag == ASUN_VAL_INT) return asun_double_eq(a->d, (double)b->i);
    if (a->tag != b->tag) return false;
    switch (a->tag) {
        case ASUN_VAL_NULL:   return true;
        case ASUN_VAL_BOOL:   return a->b == b->b;
        case ASUN_VAL_INT:    return a->i == b->i;
        case ASUN_VAL_DOUBLE: return asun_double_eq(a->d, b->d);
        case ASUN_VAL_STRING: return a->s_len == b->s_len && memcmp(a->s, b->s, a->s_len) == 0;
        case ASUN_VAL_ARRAY:
            if (a->arr_len != b->arr_len) return false;
            for (size_t k = 0; k < a->arr_len; k++)
                if (!asun_value_eq(&a->arr[k], &b->arr[k])) return false;
            return true;
    }
    return false;
}

/* ---- decoder ------------------------------------------------------------ */

/* Internal: skip ws+comments; returns false on unterminated comment block. */
static inline bool asun_skip_ws_strict(const char** pos, const char* end) {
    for (;;) {
        while (*pos < end) {
            char c = **pos;
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { (*pos)++; continue; }
            break;
        }
        if (*pos + 1 < end && (*pos)[0] == '/' && (*pos)[1] == '*') {
            *pos += 2;
            bool closed = false;
            while (*pos + 1 < end) {
                if ((*pos)[0] == '*' && (*pos)[1] == '/') { *pos += 2; closed = true; break; }
                (*pos)++;
            }
            if (!closed) return false;
            continue;
        }
        return true;
    }
}

/* Internal helpers for plain-token classification. */
static inline asun_value_t* asun_value_classify_plain(const char* s, size_t len) {
    if (len == 0) return asun_value_make_string("", 0);
    if (len == 4 && memcmp(s, "true", 4) == 0)  return asun_value_make_bool(true);
    if (len == 5 && memcmp(s, "false", 5) == 0) return asun_value_make_bool(false);

    /* Try integer */
    size_t k = 0;
    bool neg = false;
    if (s[0] == '-') { neg = true; k = 1; }
    if (k < len) {
        bool all_digits = true;
        for (size_t j = k; j < len; j++) {
            if (s[j] < '0' || s[j] > '9') { all_digits = false; break; }
        }
        if (all_digits) {
            uint64_t v = 0;
            uint64_t lim = neg ? (uint64_t)INT64_MAX + 1 : (uint64_t)INT64_MAX;
            bool overflow = false;
            for (size_t j = k; j < len; j++) {
                int dg = s[j] - '0';
                if (v > (lim - (uint64_t)dg) / 10) { overflow = true; break; }
                v = v * 10 + (uint64_t)dg;
            }
            if (!overflow) {
                int64_t i = neg ? (v == 0 ? 0 : -(int64_t)(v - 1) - 1) : (int64_t)v;
                return asun_value_make_int(i);
            }
        }
    }

    /* Try float — ABNF:
     *   float = ["-"] 1*DIGIT [ "." 1*DIGIT ] [ ("e"/"E") ["+"/"-"] 1*DIGIT ]
     * Both fractional and exponent parts (if present) MUST have ≥1 digit.
     * Leading "+" is forbidden. Tokens like "5.", ".5", "+5", "1e", "1e+"
     * therefore fall through to plain-string per the type-priority cascade.
     */
    {
        size_t j = 0;
        if (j < len && s[j] == '-') j++;
        size_t int_start = j;
        while (j < len && s[j] >= '0' && s[j] <= '9') j++;
        bool int_ok = (j > int_start);
        bool has_frac_or_exp = false;
        if (int_ok && j < len && s[j] == '.') {
            j++;
            size_t frac_start = j;
            while (j < len && s[j] >= '0' && s[j] <= '9') j++;
            if (j == frac_start) goto as_string;
            has_frac_or_exp = true;
        }
        if (int_ok && j < len && (s[j] == 'e' || s[j] == 'E')) {
            j++;
            if (j < len && (s[j] == '+' || s[j] == '-')) j++;
            size_t exp_start = j;
            while (j < len && s[j] >= '0' && s[j] <= '9') j++;
            if (j == exp_start) goto as_string;
            has_frac_or_exp = true;
        }
        if (int_ok && has_frac_or_exp && j == len) {
            char buf[64];
            size_t n = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
            memcpy(buf, s, n); buf[n] = '\0';
            char* endp = NULL;
            double dv = strtod(buf, &endp);
            if (endp && *endp == '\0') return asun_value_make_double(dv);
        }
    }
as_string:;

    /* Fallback: string with plain-token escape unwrap */
    char* out = (char*)malloc(len + 1);
    size_t oi = 0;
    for (size_t j = 0; j < len; j++) {
        char c = s[j];
        if (c == '\\' && j + 1 < len) {
            char esc = s[j + 1];
            switch (esc) {
                case 'n': out[oi++] = '\n'; j++; continue;
                case 'r': out[oi++] = '\r'; j++; continue;
                case 't': out[oi++] = '\t'; j++; continue;
                case 'b': out[oi++] = '\b'; j++; continue;
                case 'f': out[oi++] = '\f'; j++; continue;
                case '\\': out[oi++] = '\\'; j++; continue;
                case '"':  out[oi++] = '"';  j++; continue;
                case ',':  out[oi++] = ',';  j++; continue;
                case '(':  out[oi++] = '(';  j++; continue;
                case ')':  out[oi++] = ')';  j++; continue;
                case '[':  out[oi++] = '[';  j++; continue;
                case ']':  out[oi++] = ']';  j++; continue;
                case '{':  out[oi++] = '{';  j++; continue;
                case '}':  out[oi++] = '}';  j++; continue;
                case '@':  out[oi++] = '@';  j++; continue;
                case '<':  out[oi++] = '<';  j++; continue;
                case '>':  out[oi++] = '>';  j++; continue;
                case ':':  out[oi++] = ':';  j++; continue;
                case 'u':
                    if (j + 5 < len) {
                        char hex[5] = {s[j+2], s[j+3], s[j+4], s[j+5], 0};
                        unsigned long cp = strtoul(hex, NULL, 16);
                        if (cp < 0x80) out[oi++] = (char)cp;
                        else if (cp < 0x800) {
                            out[oi++] = (char)(0xC0 | (cp >> 6));
                            out[oi++] = (char)(0x80 | (cp & 0x3F));
                        } else {
                            /* worst-case: 3-byte UTF-8 may exceed initial buffer if many escapes; grow */
                            if (oi + 3 > len) { out = (char*)realloc(out, oi + 4); }
                            out[oi++] = (char)(0xE0 | (cp >> 12));
                            out[oi++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                            out[oi++] = (char)(0x80 | (cp & 0x3F));
                        }
                        j += 5;
                        continue;
                    }
                    break;
                default: break;
            }
        }
        out[oi++] = c;
    }
    out[oi] = '\0';
    asun_value_t* v = asun_value_alloc(ASUN_VAL_STRING);
    v->s = out;
    v->s_len = oi;
    return v;
}

/* Forward decls */
static asun_value_t* asun_parse_value_inner(const char** pos, const char* end, bool* ok);
static asun_value_t* asun_parse_array_value(const char** pos, const char* end, bool* ok);

/* Quoted string parser: similar to asun_parse_quoted_string but always allocates */
static asun_value_t* asun_parse_quoted_value(const char** pos, const char* end, bool* ok) {
    if (*pos >= end || **pos != '"') { *ok = false; return NULL; }
    (*pos)++;
    /* allocate dynamically as we scan */
    size_t cap = 16, len = 0;
    char* buf = (char*)malloc(cap);
    while (*pos < end) {
        char c = **pos;
        if (c == '"') { (*pos)++; break; }
        if (c == '\\') {
            (*pos)++;
            if (*pos >= end) { free(buf); *ok = false; return NULL; }
            char esc = **pos; (*pos)++;
            char w = 0;
            int ulen = 0;
            char ubytes[4] = {0};
            switch (esc) {
                case 'n': w = '\n'; break;
                case 'r': w = '\r'; break;
                case 't': w = '\t'; break;
                case 'b': w = '\b'; break;
                case 'f': w = '\f'; break;
                case '\\': w = '\\'; break;
                case '"':  w = '"';  break;
                case '/':  w = '/';  break;
                case ',':  w = ',';  break;
                case '(':  w = '(';  break;
                case ')':  w = ')';  break;
                case '[':  w = '[';  break;
                case ']':  w = ']';  break;
                case '{':  w = '{';  break;
                case '}':  w = '}';  break;
                case '@':  w = '@';  break;
                case '<':  w = '<';  break;
                case '>':  w = '>';  break;
                case ':':  w = ':';  break;
                case 'u': {
                    if (*pos + 4 > end) { free(buf); *ok = false; return NULL; }
                    char hex[5] = {(*pos)[0], (*pos)[1], (*pos)[2], (*pos)[3], 0};
                    *pos += 4;
                    unsigned long cp = strtoul(hex, NULL, 16);
                    if (cp < 0x80) { ubytes[0] = (char)cp; ulen = 1; }
                    else if (cp < 0x800) {
                        ubytes[0] = (char)(0xC0 | (cp >> 6));
                        ubytes[1] = (char)(0x80 | (cp & 0x3F));
                        ulen = 2;
                    } else {
                        ubytes[0] = (char)(0xE0 | (cp >> 12));
                        ubytes[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        ubytes[2] = (char)(0x80 | (cp & 0x3F));
                        ulen = 3;
                    }
                    break;
                }
                default:
                    free(buf); *ok = false; return NULL;
            }
            if (ulen) {
                if (len + (size_t)ulen >= cap) { while (len + (size_t)ulen >= cap) cap *= 2; buf = (char*)realloc(buf, cap); }
                memcpy(buf + len, ubytes, (size_t)ulen);
                len += (size_t)ulen;
            } else {
                if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
                buf[len++] = w;
            }
        } else {
            if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
            buf[len++] = c;
            (*pos)++;
        }
    }
    if (len + 1 >= cap) { cap += 1; buf = (char*)realloc(buf, cap); }
    buf[len] = '\0';
    asun_value_t* v = asun_value_alloc(ASUN_VAL_STRING);
    v->s = buf; v->s_len = len;
    return v;
}

static asun_value_t* asun_parse_array_value(const char** pos, const char* end, bool* ok) {
    if (*pos >= end || **pos != '[') { *ok = false; return NULL; }
    (*pos)++;
    asun_value_t* arr = asun_value_alloc(ASUN_VAL_ARRAY);
    bool first = true;
    for (;;) {
        if (!asun_skip_ws_strict(pos, end)) { asun_value_free(arr); *ok = false; return NULL; }
        if (*pos >= end) { asun_value_free(arr); *ok = false; return NULL; }
        if (**pos == ']') { (*pos)++; break; }
        if (!first) {
            if (**pos != ',') { asun_value_free(arr); *ok = false; return NULL; }
            (*pos)++;
            if (!asun_skip_ws_strict(pos, end)) { asun_value_free(arr); *ok = false; return NULL; }
            if (*pos >= end) { asun_value_free(arr); *ok = false; return NULL; }
            if (**pos == ']') { (*pos)++; break; }
        }
        first = false;
        if (**pos == ',' || **pos == ']') {
            asun_value_t* nv = asun_value_make_null();
            asun_value_array_push(arr, nv);
            continue;
        }
        asun_value_t* child = asun_parse_value_inner(pos, end, ok);
        if (!*ok) { asun_value_free(arr); return NULL; }
        asun_value_array_push(arr, child);
    }
    return arr;
}

static asun_value_t* asun_parse_value_inner(const char** pos, const char* end, bool* ok) {
    if (!asun_skip_ws_strict(pos, end)) { *ok = false; return NULL; }
    if (*pos >= end) return asun_value_make_null();
    char c = **pos;
    if (c == '[') return asun_parse_array_value(pos, end, ok);
    if (c == '"') return asun_parse_quoted_value(pos, end, ok);
    if (c == '(') {
        (*pos)++;
        if (!asun_skip_ws_strict(pos, end)) { *ok = false; return NULL; }
        if (*pos < end && **pos == ')') { (*pos)++; return asun_value_make_null(); }
        *ok = false; return NULL;
    }
    /* Plain token */
    const char* start = *pos;
    while (*pos < end) {
        char b = **pos;
        if (b == ',' || b == ']' || b == ')' || b == '}') break;
        if (b == '\\' && *pos + 1 < end) { *pos += 2; continue; }
        (*pos)++;
    }
    const char* tok_end = *pos;
    while (tok_end > start && (tok_end[-1] == ' ' || tok_end[-1] == '\t' ||
                               tok_end[-1] == '\n' || tok_end[-1] == '\r')) tok_end--;
    while (start < tok_end && (*start == ' ' || *start == '\t' ||
                               *start == '\n' || *start == '\r')) start++;
    return asun_value_classify_plain(start, (size_t)(tok_end - start));
}

/* Public top-level decoder. Returns NULL on error; *err is set if non-NULL. */
static inline asun_value_t* asun_value_decode(const char* input, size_t len) {
    const char* pos = input;
    const char* end = input + len;
    bool ok = true;
    if (!asun_skip_ws_strict(&pos, end)) return NULL;
    if (pos >= end) return asun_value_make_null();

    asun_value_t* out = NULL;
    char c = *pos;
    if (c == '(') {
        pos++;
        if (!asun_skip_ws_strict(&pos, end)) return NULL;
        if (pos < end && *pos == ')') { pos++; out = asun_value_make_null(); }
        else return NULL; /* bare tuple */
    } else if (c == '[') {
        out = asun_parse_array_value(&pos, end, &ok);
        if (!ok) return NULL;
    } else if (c == '"') {
        out = asun_parse_quoted_value(&pos, end, &ok);
        if (!ok) return NULL;
    } else {
        const char* start = pos;
        while (pos < end) {
            char b = *pos;
            if (b == ',' || b == ']' || b == ')' || b == '}') break;
            if (b == '\\' && pos + 1 < end) { pos += 2; continue; }
            pos++;
        }
        const char* tok_end = pos;
        while (tok_end > start && (tok_end[-1] == ' ' || tok_end[-1] == '\t' ||
                                   tok_end[-1] == '\n' || tok_end[-1] == '\r')) tok_end--;
        while (start < tok_end && (*start == ' ' || *start == '\t' ||
                                   *start == '\n' || *start == '\r')) start++;
        out = asun_value_classify_plain(start, (size_t)(tok_end - start));
    }

    if (!asun_skip_ws_strict(&pos, end)) { asun_value_free(out); return NULL; }
    if (pos < end) { asun_value_free(out); return NULL; }
    return out;
}

/* ---- encoder ------------------------------------------------------------ */

static inline void asun_value_encode_into(asun_buf_t* buf, const asun_value_t* v) {
    switch (v->tag) {
        case ASUN_VAL_NULL:   asun_buf_appends(buf, "()"); return;
        case ASUN_VAL_BOOL:   asun_buf_appends(buf, v->b ? "true" : "false"); return;
        case ASUN_VAL_INT:    asun_buf_append_i64(buf, v->i); return;
        case ASUN_VAL_DOUBLE: asun_buf_append_f64(buf, v->d); return;
        case ASUN_VAL_STRING: asun_buf_append_str(buf, v->s, v->s_len); return;
        case ASUN_VAL_ARRAY:
            asun_buf_push(buf, '[');
            for (size_t k = 0; k < v->arr_len; k++) {
                if (k) asun_buf_push(buf, ',');
                asun_value_encode_into(buf, &v->arr[k]);
            }
            asun_buf_push(buf, ']');
            return;
    }
}

static inline asun_buf_t asun_value_encode(const asun_value_t* v) {
    asun_buf_t buf = asun_buf_new(64);
    asun_value_encode_into(&buf, v);
    return buf;
}

#ifdef __cplusplus
}
#endif

#endif /* ASUN_VALUE_H */
