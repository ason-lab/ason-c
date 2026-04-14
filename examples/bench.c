#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "asun.h"

/* ===========================================================================
 * Timing
 * =========================================================================== */
#ifdef __APPLE__
#include <mach/mach_time.h>
static double now_ms(void) {
    static mach_timebase_info_data_t info = {0,0};
    if (info.denom == 0) mach_timebase_info(&info);
    return (double)mach_absolute_time() * info.numer / info.denom / 1e6;
}
#else
static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}
#endif

/* ===========================================================================
 * Structs
 * =========================================================================== */

typedef struct {
    int64_t id;
    asun_string_t name;
    asun_string_t email;
    int64_t age;
    double score;
    bool active;
    asun_string_t role;
    asun_string_t city;
} BUser;

ASUN_FIELDS(BUser, 8,
    ASUN_FIELD(BUser, id,     "id",     i64),
    ASUN_FIELD(BUser, name,   "name",   str),
    ASUN_FIELD(BUser, email,  "email",  str),
    ASUN_FIELD(BUser, age,    "age",    i64),
    ASUN_FIELD(BUser, score,  "score",  f64),
    ASUN_FIELD(BUser, active, "active", bool),
    ASUN_FIELD(BUser, role,   "role",   str),
    ASUN_FIELD(BUser, city,   "city",   str))

ASUN_FIELDS_BIN(BUser, 8)

typedef struct {
    bool b;
    int8_t i8v;
    int16_t i16v;
    int32_t i32v;
    int64_t i64v;
    uint8_t u8v;
    uint16_t u16v;
    uint32_t u32v;
    uint64_t u64v;
    float f32v;
    double f64v;
    asun_string_t s;
    asun_opt_i64 opt_some;
    asun_opt_i64 opt_none;
    asun_vec_i64 vec_int;
    asun_vec_str vec_str;
} BAllTypes;

ASUN_FIELDS(BAllTypes, 16,
    ASUN_FIELD(BAllTypes, b,        "b",        bool),
    ASUN_FIELD(BAllTypes, i8v,      "i8v",      i8),
    ASUN_FIELD(BAllTypes, i16v,     "i16v",     i16),
    ASUN_FIELD(BAllTypes, i32v,     "i32v",     i32),
    ASUN_FIELD(BAllTypes, i64v,     "i64v",     i64),
    ASUN_FIELD(BAllTypes, u8v,      "u8v",      u8),
    ASUN_FIELD(BAllTypes, u16v,     "u16v",     u16),
    ASUN_FIELD(BAllTypes, u32v,     "u32v",     u32),
    ASUN_FIELD(BAllTypes, u64v,     "u64v",     u64),
    ASUN_FIELD(BAllTypes, f32v,     "f32v",     f32),
    ASUN_FIELD(BAllTypes, f64v,     "f64v",     f64),
    ASUN_FIELD(BAllTypes, s,        "s",        str),
    ASUN_FIELD(BAllTypes, opt_some, "opt_some", opt_i64),
    ASUN_FIELD(BAllTypes, opt_none, "opt_none", opt_i64),
    ASUN_FIELD(BAllTypes, vec_int,  "vec_int",  vec_i64),
    ASUN_FIELD(BAllTypes, vec_str,  "vec_str",  vec_str))
ASUN_FIELDS_BIN(BAllTypes, 16)
ASUN_VEC_STRUCT_DEFINE(BAllTypes)

/* 5-level: Company > Division > Team > Project > Task */
typedef struct { int64_t id; asun_string_t title; int64_t priority; bool done; double hours; } BTask;
ASUN_FIELDS(BTask, 5,
    ASUN_FIELD(BTask, id,       "id",       i64),
    ASUN_FIELD(BTask, title,    "title",    str),
    ASUN_FIELD(BTask, priority, "priority", i64),
    ASUN_FIELD(BTask, done,     "done",     bool),
    ASUN_FIELD(BTask, hours,    "hours",    f64))
ASUN_FIELDS_BIN(BTask, 5)
ASUN_VEC_STRUCT_DEFINE(BTask)

typedef struct { asun_string_t name; double budget; bool active; asun_vec_BTask tasks; } BProject;
ASUN_FIELDS(BProject, 4,
    ASUN_FIELD(BProject, name,   "name",   str),
    ASUN_FIELD(BProject, budget, "budget", f64),
    ASUN_FIELD(BProject, active, "active", bool),
    ASUN_FIELD_VEC_STRUCT(BProject, tasks, "tasks", BTask))
ASUN_FIELDS_BIN(BProject, 4)
ASUN_VEC_STRUCT_DEFINE(BProject)

typedef struct { asun_string_t name; asun_string_t lead; int64_t size; asun_vec_BProject projects; } BTeam;
ASUN_FIELDS(BTeam, 4,
    ASUN_FIELD(BTeam, name,     "name",     str),
    ASUN_FIELD(BTeam, lead,     "lead",     str),
    ASUN_FIELD(BTeam, size,     "size",     i64),
    ASUN_FIELD_VEC_STRUCT(BTeam, projects, "projects", BProject))
ASUN_FIELDS_BIN(BTeam, 4)
ASUN_VEC_STRUCT_DEFINE(BTeam)

typedef struct { asun_string_t name; asun_string_t location; int64_t headcount; asun_vec_BTeam teams; } BDivision;
ASUN_FIELDS(BDivision, 4,
    ASUN_FIELD(BDivision, name,      "name",      str),
    ASUN_FIELD(BDivision, location,  "location",  str),
    ASUN_FIELD(BDivision, headcount, "headcount", i64),
    ASUN_FIELD_VEC_STRUCT(BDivision, teams, "teams", BTeam))
ASUN_FIELDS_BIN(BDivision, 4)
ASUN_VEC_STRUCT_DEFINE(BDivision)

typedef struct {
    asun_string_t name;
    int64_t founded;
    double revenue_m;
    bool is_public;
    asun_vec_BDivision divisions;
    asun_vec_str tags;
} BCompany;

ASUN_FIELDS(BCompany, 6,
    ASUN_FIELD(BCompany, name,      "name",      str),
    ASUN_FIELD(BCompany, founded,   "founded",   i64),
    ASUN_FIELD(BCompany, revenue_m, "revenue_m", f64),
    ASUN_FIELD(BCompany, is_public, "public",    bool),
    ASUN_FIELD_VEC_STRUCT(BCompany, divisions, "divisions", BDivision),
    ASUN_FIELD(BCompany, tags,      "tags",      vec_str))
ASUN_FIELDS_BIN(BCompany, 6)
ASUN_VEC_STRUCT_DEFINE(BCompany)

/* ===========================================================================
 * Mini JSON serializer (for comparison)
 * =========================================================================== */

static void json_append_str(asun_buf_t* b, const char* s, size_t len) {
    asun_buf_push(b, '"');
    for (size_t i = 0; i < len; i++) {
        switch (s[i]) {
        case '"':  asun_buf_append(b, "\\\"", 2); break;
        case '\\': asun_buf_append(b, "\\\\", 2); break;
        case '\n': asun_buf_append(b, "\\n", 2); break;
        case '\t': asun_buf_append(b, "\\t", 2); break;
        default:   asun_buf_push(b, s[i]); break;
        }
    }
    asun_buf_push(b, '"');
}

static void json_append_i64(asun_buf_t* b, int64_t v) { asun_buf_append_i64(b, v); }
static void json_append_u64(asun_buf_t* b, uint64_t v) { asun_buf_append_u64(b, v); }
static void json_append_f64(asun_buf_t* b, double v) { asun_buf_append_f64(b, v); }

#define JSON_KEY_STR(buf, key, s, slen) \
    asun_buf_push(buf, '"'); asun_buf_append(buf, key, strlen(key)); asun_buf_append(buf, "\":", 2); \
    json_append_str(buf, s, slen)

#define JSON_KEY_I64(buf, key, v) \
    asun_buf_push(buf, '"'); asun_buf_append(buf, key, strlen(key)); asun_buf_append(buf, "\":", 2); \
    json_append_i64(buf, v)

#define JSON_KEY_F64(buf, key, v) \
    asun_buf_push(buf, '"'); asun_buf_append(buf, key, strlen(key)); asun_buf_append(buf, "\":", 2); \
    json_append_f64(buf, v)

#define JSON_KEY_BOOL(buf, key, v) \
    asun_buf_push(buf, '"'); asun_buf_append(buf, key, strlen(key)); asun_buf_append(buf, "\":", 2); \
    asun_buf_append(buf, (v) ? "true" : "false", (v) ? 4 : 5)

static void json_serialize_user(asun_buf_t* b, const BUser* u) {
    asun_buf_push(b, '{');
    JSON_KEY_I64(b, "id", u->id); asun_buf_push(b, ',');
    JSON_KEY_STR(b, "name", u->name.data, u->name.len); asun_buf_push(b, ',');
    JSON_KEY_STR(b, "email", u->email.data, u->email.len); asun_buf_push(b, ',');
    JSON_KEY_I64(b, "age", u->age); asun_buf_push(b, ',');
    JSON_KEY_F64(b, "score", u->score); asun_buf_push(b, ',');
    JSON_KEY_BOOL(b, "active", u->active); asun_buf_push(b, ',');
    JSON_KEY_STR(b, "role", u->role.data, u->role.len); asun_buf_push(b, ',');
    JSON_KEY_STR(b, "city", u->city.data, u->city.len);
    asun_buf_push(b, '}');
}

static asun_buf_t json_serialize_users(const BUser* users, size_t n) {
    asun_buf_t b = asun_buf_new(n * 200);
    asun_buf_push(&b, '[');
    for (size_t i = 0; i < n; i++) {
        if (i > 0) asun_buf_push(&b, ',');
        json_serialize_user(&b, &users[i]);
    }
    asun_buf_push(&b, ']');
    return b;
}

static void json_serialize_alltypes_item(asun_buf_t* b, const BAllTypes* item) {
    asun_buf_push(b, '{');
    JSON_KEY_BOOL(b, "b", item->b); asun_buf_push(b, ',');
    JSON_KEY_I64(b, "i8v", item->i8v); asun_buf_push(b, ',');
    JSON_KEY_I64(b, "i16v", item->i16v); asun_buf_push(b, ',');
    JSON_KEY_I64(b, "i32v", item->i32v); asun_buf_push(b, ',');
    JSON_KEY_I64(b, "i64v", item->i64v); asun_buf_push(b, ',');
    JSON_KEY_I64(b, "u8v", item->u8v); asun_buf_push(b, ',');
    JSON_KEY_I64(b, "u16v", item->u16v); asun_buf_push(b, ',');
    JSON_KEY_I64(b, "u32v", item->u32v); asun_buf_push(b, ',');
    asun_buf_push(b, '"'); asun_buf_appends(b, "u64v\":"); json_append_u64(b, item->u64v); asun_buf_push(b, ',');
    JSON_KEY_F64(b, "f32v", item->f32v); asun_buf_push(b, ',');
    JSON_KEY_F64(b, "f64v", item->f64v); asun_buf_push(b, ',');
    JSON_KEY_STR(b, "s", item->s.data, item->s.len); asun_buf_push(b, ',');
    asun_buf_append(b, "\"opt_some\":", 11);
    if (item->opt_some.has_value) json_append_i64(b, item->opt_some.value);
    else asun_buf_appends(b, "null");
    asun_buf_push(b, ',');
    asun_buf_append(b, "\"opt_none\":", 11);
    if (item->opt_none.has_value) json_append_i64(b, item->opt_none.value);
    else asun_buf_appends(b, "null");
    asun_buf_push(b, ',');
    asun_buf_append(b, "\"vec_int\":[", 11);
    for (size_t i = 0; i < item->vec_int.len; i++) {
        if (i > 0) asun_buf_push(b, ',');
        json_append_i64(b, item->vec_int.data[i]);
    }
    asun_buf_push(b, ']');
    asun_buf_push(b, ',');
    asun_buf_append(b, "\"vec_str\":[", 11);
    for (size_t i = 0; i < item->vec_str.len; i++) {
        if (i > 0) asun_buf_push(b, ',');
        json_append_str(b, item->vec_str.data[i].data, item->vec_str.data[i].len);
    }
    asun_buf_append(b, "]}", 2);
}

static asun_buf_t json_serialize_alltypes(const BAllTypes* items, size_t n) {
    asun_buf_t b = asun_buf_new(n * 256);
    asun_buf_push(&b, '[');
    for (size_t i = 0; i < n; i++) {
        if (i > 0) asun_buf_push(&b, ',');
        json_serialize_alltypes_item(&b, &items[i]);
    }
    asun_buf_push(&b, ']');
    return b;
}

/* Mini JSON deserializer */
static void json_skip_ws(const char** p, const char* e) {
    while (*p < e && (**p == ' ' || **p == '\n' || **p == '\t' || **p == '\r')) (*p)++;
}
static void json_expect(const char** p, const char* e, char c) {
    json_skip_ws(p, e); if (*p < e && **p == c) (*p)++;
}
static asun_string_t json_read_str(const char** p, const char* e) {
    json_skip_ws(p, e);
    if (**p == '"') (*p)++;
    asun_buf_t b = asun_buf_new(32);
    while (*p < e && **p != '"') {
        if (**p == '\\') { (*p)++;
            if (**p == 'n') asun_buf_push(&b, '\n');
            else if (**p == 't') asun_buf_push(&b, '\t');
            else asun_buf_push(&b, **p);
            (*p)++;
        } else { asun_buf_push(&b, **p); (*p)++; }
    }
    if (*p < e) (*p)++;
    asun_string_t s = asun_string_from_len(b.data, b.len);
    asun_buf_free(&b);
    return s;
}
static int64_t json_read_i64(const char** p, const char* e) {
    json_skip_ws(p, e);
    char* endptr = NULL;
    int64_t v = strtoll(*p, &endptr, 10);
    *p = endptr; return v;
}
static uint64_t json_read_u64(const char** p, const char* e) {
    json_skip_ws(p, e);
    char* endptr = NULL;
    uint64_t v = strtoull(*p, &endptr, 10);
    *p = endptr; return v;
}
static double json_read_f64(const char** p, const char* e) {
    json_skip_ws(p, e);
    char* endptr = NULL;
    double v = strtod(*p, &endptr);
    *p = endptr; return v;
}
static bool json_read_null(const char** p, const char* e) {
    json_skip_ws(p, e);
    if (*p + 4 <= e && memcmp(*p, "null", 4) == 0) {
        *p += 4;
        return true;
    }
    return false;
}
static bool json_read_bool(const char** p, const char* e) {
    json_skip_ws(p, e);
    if (*p + 4 <= e && (*p)[0] == 't') { *p += 4; return true; }
    *p += 5; return false;
}
static void json_skip_comma(const char** p, const char* e) {
    json_skip_ws(p, e); if (*p < e && **p == ',') (*p)++;
}
static void json_skip_key(const char** p, const char* e) {
    asun_string_t k = json_read_str(p, e);
    asun_string_free(&k);
    json_expect(p, e, ':');
}

static BUser json_deserialize_user(const char** p, const char* e) {
    BUser u = {0};
    json_expect(p, e, '{');
    json_skip_key(p, e); u.id = json_read_i64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); u.name = json_read_str(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); u.email = json_read_str(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); u.age = json_read_i64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); u.score = json_read_f64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); u.active = json_read_bool(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); u.role = json_read_str(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); u.city = json_read_str(p, e);
    json_expect(p, e, '}');
    return u;
}

static BUser* json_deserialize_users(const char* data, size_t len, size_t* out_n) {
    const char* p = data;
    const char* e = data + len;
    size_t cap = 64, cnt = 0;
    BUser* arr = (BUser*)malloc(cap * sizeof(BUser));
    json_expect(&p, e, '[');
    while (1) {
        json_skip_ws(&p, e);
        if (p >= e || *p == ']') break;
        if (cnt > 0) json_skip_comma(&p, e);
        if (cnt >= cap) { cap *= 2; arr = (BUser*)realloc(arr, cap * sizeof(BUser)); }
        arr[cnt++] = json_deserialize_user(&p, e);
    }
    *out_n = cnt;
    return arr;
}

static asun_vec_i64 json_read_i64_array(const char** p, const char* e) {
    asun_vec_i64 out = asun_vec_i64_new();
    json_expect(p, e, '[');
    while (1) {
        json_skip_ws(p, e);
        if (*p >= e || **p == ']') break;
        if (out.len > 0) json_skip_comma(p, e);
        asun_vec_i64_push(&out, json_read_i64(p, e));
    }
    json_expect(p, e, ']');
    return out;
}

static asun_vec_str json_read_str_array(const char** p, const char* e) {
    asun_vec_str out = asun_vec_str_new();
    json_expect(p, e, '[');
    while (1) {
        json_skip_ws(p, e);
        if (*p >= e || **p == ']') break;
        if (out.len > 0) json_skip_comma(p, e);
        asun_vec_str_push(&out, json_read_str(p, e));
    }
    json_expect(p, e, ']');
    return out;
}

static BAllTypes json_deserialize_alltypes(const char** p, const char* e) {
    BAllTypes item = {0};
    json_expect(p, e, '{');
    json_skip_key(p, e); item.b = json_read_bool(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.i8v = (int8_t)json_read_i64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.i16v = (int16_t)json_read_i64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.i32v = (int32_t)json_read_i64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.i64v = json_read_i64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.u8v = (uint8_t)json_read_u64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.u16v = (uint16_t)json_read_u64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.u32v = (uint32_t)json_read_u64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.u64v = json_read_u64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.f32v = (float)json_read_f64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.f64v = json_read_f64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.s = json_read_str(p, e); json_skip_comma(p, e);
    json_skip_key(p, e);
    if (json_read_null(p, e)) item.opt_some = (asun_opt_i64){false, 0};
    else item.opt_some = (asun_opt_i64){true, json_read_i64(p, e)};
    json_skip_comma(p, e);
    json_skip_key(p, e);
    if (json_read_null(p, e)) item.opt_none = (asun_opt_i64){false, 0};
    else item.opt_none = (asun_opt_i64){true, json_read_i64(p, e)};
    json_skip_comma(p, e);
    json_skip_key(p, e); item.vec_int = json_read_i64_array(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); item.vec_str = json_read_str_array(p, e);
    json_expect(p, e, '}');
    return item;
}

static BAllTypes* json_deserialize_alltypes_vec(const char* data, size_t len, size_t* out_n) {
    const char* p = data;
    const char* e = data + len;
    size_t cap = 32, cnt = 0;
    BAllTypes* arr = (BAllTypes*)malloc(cap * sizeof(BAllTypes));
    json_expect(&p, e, '[');
    while (1) {
        json_skip_ws(&p, e);
        if (p >= e || *p == ']') break;
        if (cnt > 0) json_skip_comma(&p, e);
        if (cnt >= cap) {
            cap *= 2;
            arr = (BAllTypes*)realloc(arr, cap * sizeof(BAllTypes));
        }
        arr[cnt++] = json_deserialize_alltypes(&p, e);
    }
    *out_n = cnt;
    return arr;
}

/* JSON deep struct deserializers (for fair comparison) */

static BTask json_deserialize_task(const char** p, const char* e) {
    BTask t = {0};
    json_expect(p, e, '{');
    json_skip_key(p, e); t.id = json_read_i64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); t.title = json_read_str(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); t.priority = json_read_i64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); t.done = json_read_bool(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); t.hours = json_read_f64(p, e);
    json_expect(p, e, '}');
    return t;
}

static BProject json_deserialize_project(const char** p, const char* e) {
    BProject proj = {0};
    json_expect(p, e, '{');
    json_skip_key(p, e); proj.name = json_read_str(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); proj.budget = json_read_f64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); proj.active = json_read_bool(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); /* "tasks" */
    json_expect(p, e, '[');
    proj.tasks = asun_vec_BTask_new();
    while (1) {
        json_skip_ws(p, e);
        if (*p >= e || **p == ']') break;
        if (proj.tasks.len > 0) json_skip_comma(p, e);
        asun_vec_BTask_push(&proj.tasks, json_deserialize_task(p, e));
    }
    json_expect(p, e, ']');
    json_expect(p, e, '}');
    return proj;
}

static BTeam json_deserialize_team(const char** p, const char* e) {
    BTeam team = {0};
    json_expect(p, e, '{');
    json_skip_key(p, e); team.name = json_read_str(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); team.lead = json_read_str(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); team.size = json_read_i64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); /* "projects" */
    json_expect(p, e, '[');
    team.projects = asun_vec_BProject_new();
    while (1) {
        json_skip_ws(p, e);
        if (*p >= e || **p == ']') break;
        if (team.projects.len > 0) json_skip_comma(p, e);
        asun_vec_BProject_push(&team.projects, json_deserialize_project(p, e));
    }
    json_expect(p, e, ']');
    json_expect(p, e, '}');
    return team;
}

static BDivision json_deserialize_division(const char** p, const char* e) {
    BDivision div = {0};
    json_expect(p, e, '{');
    json_skip_key(p, e); div.name = json_read_str(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); div.location = json_read_str(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); div.headcount = json_read_i64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); /* "teams" */
    json_expect(p, e, '[');
    div.teams = asun_vec_BTeam_new();
    while (1) {
        json_skip_ws(p, e);
        if (*p >= e || **p == ']') break;
        if (div.teams.len > 0) json_skip_comma(p, e);
        asun_vec_BTeam_push(&div.teams, json_deserialize_team(p, e));
    }
    json_expect(p, e, ']');
    json_expect(p, e, '}');
    return div;
}

static BCompany json_deserialize_company(const char** p, const char* e) {
    BCompany c = {0};
    json_expect(p, e, '{');
    json_skip_key(p, e); c.name = json_read_str(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); c.founded = json_read_i64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); c.revenue_m = json_read_f64(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); c.is_public = json_read_bool(p, e); json_skip_comma(p, e);
    json_skip_key(p, e); /* "divisions" */
    json_expect(p, e, '[');
    c.divisions = asun_vec_BDivision_new();
    while (1) {
        json_skip_ws(p, e);
        if (*p >= e || **p == ']') break;
        if (c.divisions.len > 0) json_skip_comma(p, e);
        asun_vec_BDivision_push(&c.divisions, json_deserialize_division(p, e));
    }
    json_expect(p, e, ']');
    json_skip_comma(p, e);
    json_skip_key(p, e); /* "tags" */
    json_expect(p, e, '[');
    c.tags = asun_vec_str_new();
    while (1) {
        json_skip_ws(p, e);
        if (*p >= e || **p == ']') break;
        if (c.tags.len > 0) json_skip_comma(p, e);
        asun_vec_str_push(&c.tags, json_read_str(p, e));
    }
    json_expect(p, e, ']');
    json_expect(p, e, '}');
    return c;
}

static BCompany* json_deserialize_companies(const char* data, size_t len, size_t* out_n) {
    const char* p = data;
    const char* e = data + len;
    size_t cap = 16, cnt = 0;
    BCompany* arr = (BCompany*)malloc(cap * sizeof(BCompany));
    json_expect(&p, e, '[');
    while (1) {
        json_skip_ws(&p, e);
        if (p >= e || *p == ']') break;
        if (cnt > 0) json_skip_comma(&p, e);
        if (cnt >= cap) { cap *= 2; arr = (BCompany*)realloc(arr, cap * sizeof(BCompany)); }
        arr[cnt++] = json_deserialize_company(&p, e);
    }
    *out_n = cnt;
    return arr;
}

static void free_buser(BUser* u) {
    asun_string_free(&u->name); asun_string_free(&u->email);
    asun_string_free(&u->role); asun_string_free(&u->city);
}

static void free_balltypes(BAllTypes* a) {
    asun_string_free(&a->s);
    asun_vec_i64_free(&a->vec_int);
    for (size_t i = 0; i < a->vec_str.len; i++) asun_string_free(&a->vec_str.data[i]);
    asun_vec_str_free(&a->vec_str);
}

static void free_btask(BTask* t) { asun_string_free(&t->title); }
static void free_bproject(BProject* p) {
    asun_string_free(&p->name);
    for (size_t i = 0; i < p->tasks.len; i++) free_btask(&p->tasks.data[i]);
    asun_vec_BTask_free(&p->tasks);
}
static void free_bteam(BTeam* t) {
    asun_string_free(&t->name); asun_string_free(&t->lead);
    for (size_t i = 0; i < t->projects.len; i++) free_bproject(&t->projects.data[i]);
    asun_vec_BProject_free(&t->projects);
}
static void free_bdivision(BDivision* d) {
    asun_string_free(&d->name); asun_string_free(&d->location);
    for (size_t i = 0; i < d->teams.len; i++) free_bteam(&d->teams.data[i]);
    asun_vec_BTeam_free(&d->teams);
}
static void free_bcompany(BCompany* c) {
    asun_string_free(&c->name);
    for (size_t i = 0; i < c->divisions.len; i++) free_bdivision(&c->divisions.data[i]);
    asun_vec_BDivision_free(&c->divisions);
    for (size_t i = 0; i < c->tags.len; i++) asun_string_free(&c->tags.data[i]);
    asun_vec_str_free(&c->tags);
}

/* ===========================================================================
 * Data generators
 * =========================================================================== */

static BUser* generate_users(size_t n) {
    const char* names[] = {"Alice","Bob","Carol","David","Eve","Frank","Grace","Hank"};
    const char* roles[] = {"engineer","designer","manager","analyst"};
    const char* cities[] = {"NYC","LA","Chicago","Houston","Phoenix"};
    BUser* users = (BUser*)calloc(n, sizeof(BUser));
    for (size_t i = 0; i < n; i++) {
        users[i].id = (int64_t)i;
        users[i].name = asun_string_from(names[i % 8]);
        char email[64]; snprintf(email, 64, "%s@example.com", names[i % 8]);
        users[i].email = asun_string_from(email);
        users[i].age = 25 + (int64_t)(i % 40);
        users[i].score = 50.0 + (double)(i % 50) + 0.5;
        users[i].active = (i % 3 != 0);
        users[i].role = asun_string_from(roles[i % 4]);
        users[i].city = asun_string_from(cities[i % 5]);
    }
    return users;
}

static BAllTypes* generate_all_types(size_t n) {
    BAllTypes* items = (BAllTypes*)calloc(n, sizeof(BAllTypes));
    for (size_t i = 0; i < n; i++) {
        items[i].b = (i % 2 == 0);
        items[i].i8v = (int8_t)(i % 256);
        items[i].i16v = -(int16_t)i;
        items[i].i32v = (int32_t)(i * 1000);
        items[i].i64v = (int64_t)(i * 100000);
        items[i].u8v = (uint8_t)(i % 256);
        items[i].u16v = (uint16_t)(i % 65536);
        items[i].u32v = (uint32_t)(i * 7919);
        items[i].u64v = (uint64_t)(i * 1000000007ULL);
        items[i].f32v = (float)i * 1.5f;
        items[i].f64v = (double)i * 0.25 + 0.5;
        char tmp[32]; snprintf(tmp, 32, "item_%zu", i);
        items[i].s = asun_string_from(tmp);
        items[i].opt_some = (i % 2 == 0) ? (asun_opt_i64){true, (int64_t)i} : (asun_opt_i64){false, 0};
        items[i].opt_none = (asun_opt_i64){false, 0};
        items[i].vec_int = asun_vec_i64_new();
        asun_vec_i64_push(&items[i].vec_int, (int64_t)i);
        asun_vec_i64_push(&items[i].vec_int, (int64_t)(i+1));
        asun_vec_i64_push(&items[i].vec_int, (int64_t)(i+2));
        items[i].vec_str = asun_vec_str_new();
        char t1[16], t2[16]; snprintf(t1, 16, "tag%zu", i%5); snprintf(t2, 16, "cat%zu", i%3);
        asun_vec_str_push(&items[i].vec_str, asun_string_from(t1));
        asun_vec_str_push(&items[i].vec_str, asun_string_from(t2));
    }
    return items;
}

static BCompany* generate_companies(size_t n) {
    const int dp = 2, tp = 2, pp = 3, tkp = 4;
    const char* locs[] = {"NYC","London","Tokyo","Berlin"};
    const char* leads[] = {"Alice","Bob","Carol","David"};
    BCompany* companies = (BCompany*)calloc(n, sizeof(BCompany));
    for (size_t i = 0; i < n; i++) {
        char tmp[64];
        snprintf(tmp, 64, "Corp_%zu", i); companies[i].name = asun_string_from(tmp);
        companies[i].founded = 1990 + (int64_t)(i % 35);
        companies[i].revenue_m = 10.0 + (double)i * 5.5;
        companies[i].is_public = (i % 2 == 0);
        companies[i].tags = asun_vec_str_new();
        asun_vec_str_push(&companies[i].tags, asun_string_from("enterprise"));
        asun_vec_str_push(&companies[i].tags, asun_string_from("tech"));
        snprintf(tmp, 64, "sector_%zu", i % 5);
        asun_vec_str_push(&companies[i].tags, asun_string_from(tmp));
        companies[i].divisions = asun_vec_BDivision_new();
        for (int d = 0; d < dp; d++) {
            BDivision div = {0};
            snprintf(tmp, 64, "Div_%zu_%d", i, d); div.name = asun_string_from(tmp);
            div.location = asun_string_from(locs[d % 4]);
            div.headcount = 50 + (int64_t)(d * 20);
            div.teams = asun_vec_BTeam_new();
            for (int t = 0; t < tp; t++) {
                BTeam team = {0};
                snprintf(tmp, 64, "Team_%zu_%d_%d", i, d, t); team.name = asun_string_from(tmp);
                team.lead = asun_string_from(leads[t % 4]);
                team.size = 5 + (int64_t)(t * 2);
                team.projects = asun_vec_BProject_new();
                for (int p = 0; p < pp; p++) {
                    BProject proj = {0};
                    snprintf(tmp, 64, "Proj_%d_%d", t, p); proj.name = asun_string_from(tmp);
                    proj.budget = 100.0 + (double)p * 50.5;
                    proj.active = (p % 2 == 0);
                    proj.tasks = asun_vec_BTask_new();
                    for (int tk = 0; tk < tkp; tk++) {
                        BTask task = {0};
                        task.id = (int64_t)(i * 100 + d * 10 + t * 5 + tk);
                        snprintf(tmp, 64, "Task_%d", tk); task.title = asun_string_from(tmp);
                        task.priority = (int64_t)(tk % 3 + 1);
                        task.done = (tk % 2 == 0);
                        task.hours = 2.0 + (double)tk * 1.5;
                        asun_vec_BTask_push(&proj.tasks, task);
                    }
                    asun_vec_BProject_push(&team.projects, proj);
                }
                asun_vec_BTeam_push(&div.teams, team);
            }
            asun_vec_BDivision_push(&companies[i].divisions, div);
        }
    }
    return companies;
}

/* ===========================================================================
 * JSON serialization for Company (for comparison)
 * =========================================================================== */
static void json_serialize_company(asun_buf_t* b, const BCompany* c) {
    asun_buf_push(b, '{');
    JSON_KEY_STR(b, "name", c->name.data, c->name.len); asun_buf_push(b, ',');
    JSON_KEY_I64(b, "founded", c->founded); asun_buf_push(b, ',');
    JSON_KEY_F64(b, "revenue_m", c->revenue_m); asun_buf_push(b, ',');
    JSON_KEY_BOOL(b, "public", c->is_public); asun_buf_push(b, ',');
    asun_buf_append(b, "\"divisions\":[", 13);
    for (size_t d = 0; d < c->divisions.len; d++) {
        if (d > 0) asun_buf_push(b, ',');
        const BDivision* div = &c->divisions.data[d];
        asun_buf_push(b, '{');
        JSON_KEY_STR(b, "name", div->name.data, div->name.len); asun_buf_push(b, ',');
        JSON_KEY_STR(b, "location", div->location.data, div->location.len); asun_buf_push(b, ',');
        JSON_KEY_I64(b, "headcount", div->headcount); asun_buf_push(b, ',');
        asun_buf_append(b, "\"teams\":[", 9);
        for (size_t t = 0; t < div->teams.len; t++) {
            if (t > 0) asun_buf_push(b, ',');
            const BTeam* team = &div->teams.data[t];
            asun_buf_push(b, '{');
            JSON_KEY_STR(b, "name", team->name.data, team->name.len); asun_buf_push(b, ',');
            JSON_KEY_STR(b, "lead", team->lead.data, team->lead.len); asun_buf_push(b, ',');
            JSON_KEY_I64(b, "size", team->size); asun_buf_push(b, ',');
            asun_buf_append(b, "\"projects\":[", 12);
            for (size_t p = 0; p < team->projects.len; p++) {
                if (p > 0) asun_buf_push(b, ',');
                const BProject* proj = &team->projects.data[p];
                asun_buf_push(b, '{');
                JSON_KEY_STR(b, "name", proj->name.data, proj->name.len); asun_buf_push(b, ',');
                JSON_KEY_F64(b, "budget", proj->budget); asun_buf_push(b, ',');
                JSON_KEY_BOOL(b, "active", proj->active); asun_buf_push(b, ',');
                asun_buf_append(b, "\"tasks\":[", 9);
                for (size_t tk = 0; tk < proj->tasks.len; tk++) {
                    if (tk > 0) asun_buf_push(b, ',');
                    const BTask* task = &proj->tasks.data[tk];
                    asun_buf_push(b, '{');
                    JSON_KEY_I64(b, "id", task->id); asun_buf_push(b, ',');
                    JSON_KEY_STR(b, "title", task->title.data, task->title.len); asun_buf_push(b, ',');
                    JSON_KEY_I64(b, "priority", task->priority); asun_buf_push(b, ',');
                    JSON_KEY_BOOL(b, "done", task->done); asun_buf_push(b, ',');
                    JSON_KEY_F64(b, "hours", task->hours);
                    asun_buf_push(b, '}');
                }
                asun_buf_append(b, "]}", 2);
            }
            asun_buf_append(b, "]}", 2);
        }
        asun_buf_append(b, "]}", 2);
    }
    asun_buf_append(b, "],\"tags\":[", 10);
    for (size_t i = 0; i < c->tags.len; i++) {
        if (i > 0) asun_buf_push(b, ',');
        json_append_str(b, c->tags.data[i].data, c->tags.data[i].len);
    }
    asun_buf_append(b, "]}", 2);
}

/* ===========================================================================
 * Benchmark result printing
 * =========================================================================== */

typedef struct {
    const char* name;
    double json_ser_ms, asun_ser_ms, asun_bin_ser_ms;
    double json_de_ms,  asun_de_ms,  asun_bin_de_ms;
    size_t json_bytes, asun_bytes, asun_bin_bytes;
} BenchResult;

static void format_ratio(char* out, size_t out_sz, double ratio) {
    long tenths = (long)(ratio * 10.0 + 0.5);
    if (tenths % 10 == 0) snprintf(out, out_sz, "%ldx", tenths / 10);
    else snprintf(out, out_sz, "%.1fx", tenths / 10.0);
}

static void format_percent(char* out, size_t out_sz, double pct_value) {
    long pct = (long)(pct_value + 0.5);
    snprintf(out, out_sz, "%ld%%", pct);
}

static void print_result(const BenchResult* r) {
    double asun_ser_ratio = r->json_ser_ms / r->asun_ser_ms;
    double bin_ser_ratio  = r->json_ser_ms / r->asun_bin_ser_ms;
    double asun_de_ratio  = r->json_de_ms  / r->asun_de_ms;
    double bin_de_ratio   = r->json_de_ms  / r->asun_bin_de_ms;
    double asun_percent   = ((double)r->asun_bytes / (double)r->json_bytes) * 100.0;
    double bin_percent    = ((double)r->asun_bin_bytes / (double)r->json_bytes) * 100.0;
    char asun_ser_ratio_buf[16], bin_ser_ratio_buf[16];
    char asun_de_ratio_buf[16], bin_de_ratio_buf[16];
    char asun_percent_buf[16], bin_percent_buf[16];
    format_ratio(asun_ser_ratio_buf, sizeof(asun_ser_ratio_buf), asun_ser_ratio);
    format_ratio(bin_ser_ratio_buf, sizeof(bin_ser_ratio_buf), bin_ser_ratio);
    format_ratio(asun_de_ratio_buf, sizeof(asun_de_ratio_buf), asun_de_ratio);
    format_ratio(bin_de_ratio_buf, sizeof(bin_de_ratio_buf), bin_de_ratio);
    format_percent(asun_percent_buf, sizeof(asun_percent_buf), asun_percent);
    format_percent(bin_percent_buf, sizeof(bin_percent_buf), bin_percent);
    printf("  %s\n", r->name);
    printf("    Serialize:   JSON %8.2fms | ASUN %8.2fms (%s) | BIN %8.2fms (%s)\n",
           r->json_ser_ms, r->asun_ser_ms, asun_ser_ratio_buf, r->asun_bin_ser_ms, bin_ser_ratio_buf);
    printf("    Deserialize: JSON %8.2fms | ASUN %8.2fms (%s) | BIN %8.2fms (%s)\n",
           r->json_de_ms, r->asun_de_ms, asun_de_ratio_buf, r->asun_bin_de_ms, bin_de_ratio_buf);
    printf("    Size:        JSON %8zu B | ASUN %8zu B (%s) | BIN %8zu B (%s)\n",
           r->json_bytes, r->asun_bytes, asun_percent_buf, r->asun_bin_bytes, bin_percent_buf);
}

/* ===========================================================================
 * Benchmarks
 * =========================================================================== */

static BenchResult bench_flat(size_t count, int iterations) {
    BUser* users = generate_users(count);
    char name_buf[128];
    snprintf(name_buf, 128, "Flat struct x %zu (8 fields)", count);

    /* JSON serialize */
    asun_buf_t json_buf = {0};
    double t0 = now_ms();
    for (int i = 0; i < iterations; i++) {
        asun_buf_free(&json_buf);
        json_buf = json_serialize_users(users, count);
    }
    double json_ser = now_ms() - t0;

    /* ASUN serialize */
    asun_buf_t asun_buf = {0};
    t0 = now_ms();
    for (int i = 0; i < iterations; i++) {
        asun_buf_free(&asun_buf);
        asun_buf = asun_encode_vec_BUser(users, count);
    }
    double asun_ser = now_ms() - t0;

    /* JSON deserialize */
    t0 = now_ms();
    for (int i = 0; i < iterations; i++) {
        size_t n = 0;
        BUser* r = json_deserialize_users(json_buf.data, json_buf.len, &n);
        assert(n == count);
        for (size_t j = 0; j < n; j++) free_buser(&r[j]);
        free(r);
    }
    double json_de = now_ms() - t0;

    /* ASUN deserialize */
    t0 = now_ms();
    for (int i = 0; i < iterations; i++) {
        size_t n = 0;
        BUser* r = NULL;
        asun_err_t err = asun_decode_vec_BUser(asun_buf.data, asun_buf.len, &r, &n);
        assert(err == ASUN_OK);
        assert(n == count);
        for (size_t j = 0; j < n; j++) free_buser(&r[j]);
        free(r);
    }
    double asun_de = now_ms() - t0;

    /* ASUN-BIN serialize */
    asun_buf_t asun_bin_buf = {0};
    t0 = now_ms();
    for (int i = 0; i < iterations; i++) {
        asun_buf_free(&asun_bin_buf);
        asun_bin_buf = asun_encode_bin_vec_BUser(users, count);
    }
    double asun_bin_ser = now_ms() - t0;

    /* ASUN-BIN deserialize (zero-copy: strings point into asun_bin_buf.data) */
    t0 = now_ms();
    for (int i = 0; i < iterations; i++) {
        size_t n = 0;
        BUser* r = NULL;
        asun_err_t err = asun_decode_bin_vec_BUser(asun_bin_buf.data, asun_bin_buf.len, &r, &n);
        assert(err == ASUN_OK);
        assert(n == count);
        for (size_t j = 0; j < n; j++) free_buser(&r[j]);
        free(r);
    }
    double asun_bin_de = now_ms() - t0;

    BenchResult res = {strdup(name_buf),
        json_ser, asun_ser, asun_bin_ser,
        json_de,  asun_de,  asun_bin_de,
        json_buf.len, asun_buf.len, asun_bin_buf.len};
    asun_buf_free(&json_buf);
    asun_buf_free(&asun_buf);
    asun_buf_free(&asun_bin_buf);
    for (size_t i = 0; i < count; i++) free_buser(&users[i]);
    free(users);
    return res;
}

static BenchResult bench_all_types(size_t count, int iterations) {
    BAllTypes* items = generate_all_types(count);
    char name_buf[128];
    snprintf(name_buf, 128, "All-types struct x %zu (16 fields, vec)", count);

    /* JSON serialize (full 16-field payload) */
    asun_buf_t json_buf = {0};
    double t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        asun_buf_free(&json_buf);
        json_buf = json_serialize_alltypes(items, count);
    }
    double json_ser = now_ms() - t0;

    /* ASUN serialize as one schema-driven vector payload */
    asun_buf_t asun_buf = {0};
    t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        asun_buf_free(&asun_buf);
        asun_buf = asun_encode_vec_BAllTypes(items, count);
    }
    double asun_ser = now_ms() - t0;

    /* JSON deserialize */
    t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        size_t out_count = 0;
        BAllTypes* out = json_deserialize_alltypes_vec(json_buf.data, json_buf.len, &out_count);
        assert(out_count == count);
        for (size_t i = 0; i < out_count; i++) {
            free_balltypes(&out[i]);
        }
        free(out);
    }
    double json_de = now_ms() - t0;

    /* ASUN deserialize */
    t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        BAllTypes* out = NULL;
        size_t out_count = 0;
        asun_err_t err = asun_decode_vec_BAllTypes(asun_buf.data, asun_buf.len, &out, &out_count);
        assert(err == ASUN_OK);
        assert(out_count == count);
        for (size_t i = 0; i < out_count; i++) {
            free_balltypes(&out[i]);
        }
        free(out);
    }
    double asun_de = now_ms() - t0;

    /* ASUN-BIN serialize as one vector payload */
    asun_buf_t asun_bin_buf = {0};
    t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        asun_buf_free(&asun_bin_buf);
        asun_bin_buf = asun_encode_bin_vec_BAllTypes(items, count);
    }
    double asun_bin_ser = now_ms() - t0;

    /* ASUN-BIN deserialize */
    t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        BAllTypes* out = NULL;
        size_t out_count = 0;
        asun_err_t err = asun_decode_bin_vec_BAllTypes(asun_bin_buf.data, asun_bin_buf.len, &out, &out_count);
        assert(err == ASUN_OK);
        assert(out_count == count);
        for (size_t i = 0; i < out_count; i++) {
            free_balltypes(&out[i]);
        }
        free(out);
    }
    double asun_bin_de = now_ms() - t0;

    BenchResult res = {strdup(name_buf),
        json_ser, asun_ser, asun_bin_ser,
        json_de,  asun_de,  asun_bin_de,
        json_buf.len, asun_buf.len, asun_bin_buf.len};
    asun_buf_free(&json_buf);
    asun_buf_free(&asun_buf);
    asun_buf_free(&asun_bin_buf);
    for (size_t i = 0; i < count; i++) free_balltypes(&items[i]);
    free(items);
    return res;
}

static BenchResult bench_deep(size_t count, int iterations) {
    BCompany* companies = generate_companies(count);
    char name_buf[128];
    snprintf(name_buf, 128, "5-level deep x %zu (Company>Division>Team>Project>Task)", count);

    /* JSON serialize */
    asun_buf_t json_buf = asun_buf_new(count * 4096);
    double t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        json_buf.len = 0;
        asun_buf_push(&json_buf, '[');
        for (size_t i = 0; i < count; i++) {
            if (i > 0) asun_buf_push(&json_buf, ',');
            json_serialize_company(&json_buf, &companies[i]);
        }
        asun_buf_push(&json_buf, ']');
    }
    double json_ser = now_ms() - t0;

    /* ASUN serialize as one schema-driven vector payload */
    asun_buf_t asun_buf = {0};
    t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        asun_buf_free(&asun_buf);
        asun_buf = asun_encode_vec_BCompany(companies, count);
    }
    double asun_ser = now_ms() - t0;

    /* JSON deserialize */
    t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        size_t n = 0;
        BCompany* r = json_deserialize_companies(json_buf.data, json_buf.len, &n);
        assert(n == count);
        for (size_t i = 0; i < n; i++) free_bcompany(&r[i]);
        free(r);
    }
    double json_de = now_ms() - t0;

    /* ASUN deserialize */
    t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        BCompany* out = NULL;
        size_t out_count = 0;
        asun_err_t err = asun_decode_vec_BCompany(asun_buf.data, asun_buf.len, &out, &out_count);
        assert(err == ASUN_OK);
        assert(out_count == count);
        for (size_t i = 0; i < out_count; i++) {
            free_bcompany(&out[i]);
        }
        free(out);
    }
    double asun_de = now_ms() - t0;

    /* ASUN-BIN serialize as one vector payload */
    asun_buf_t asun_bin_buf = {0};
    t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        asun_buf_free(&asun_bin_buf);
        asun_bin_buf = asun_encode_bin_vec_BCompany(companies, count);
    }
    double asun_bin_ser = now_ms() - t0;

    /* ASUN-BIN deserialize */
    t0 = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        BCompany* out = NULL;
        size_t out_count = 0;
        asun_err_t err = asun_decode_bin_vec_BCompany(asun_bin_buf.data, asun_bin_buf.len, &out, &out_count);
        assert(err == ASUN_OK);
        assert(out_count == count);
        for (size_t i = 0; i < out_count; i++) {
            free_bcompany(&out[i]);
        }
        free(out);
    }
    double asun_bin_de = now_ms() - t0;

    /* Verify */
    {
        BCompany* out = NULL;
        size_t out_count = 0;
        asun_err_t err = asun_decode_vec_BCompany(asun_buf.data, asun_buf.len, &out, &out_count);
        assert(err == ASUN_OK);
        assert(out_count == count);
        for (size_t i = 0; i < out_count; i++) {
            assert(strcmp(out[i].name.data, companies[i].name.data) == 0);
            free_bcompany(&out[i]);
        }
        free(out);
    }

    BenchResult res = {strdup(name_buf),
        json_ser, asun_ser, asun_bin_ser,
        json_de,  asun_de,  asun_bin_de,
        json_buf.len, asun_buf.len, asun_bin_buf.len};
    asun_buf_free(&json_buf);
    asun_buf_free(&asun_buf);
    asun_buf_free(&asun_bin_buf);
    for (size_t i = 0; i < count; i++) free_bcompany(&companies[i]);
    free(companies);
    return res;
}

/* ===========================================================================
 * Main
 * =========================================================================== */

int main(void) {
    printf("\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97\n");
    printf("\xe2\x95\x91          ASUN vs JSON Comprehensive Benchmark (C)          \xe2\x95\x91\n");
    printf("\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\n");

#if defined(ASUN_NEON)
    printf("\nSystem: macOS arm64 (NEON SIMD)\n");
#elif defined(ASUN_SSE2)
    printf("\nSystem: x86_64 (SSE2 SIMD)\n");
#else
    printf("\nSystem: unknown (scalar fallback)\n");
#endif

    int iterations = 100;
    printf("Iterations per test: %d\n", iterations);

    /* Section 1: Flat struct */
    printf("\n--- Section 1: Flat Struct (schema-driven vec) ---\n\n");
    {
        size_t counts[] = {100, 500, 1000, 5000};
        for (int c = 0; c < 4; c++) {
            BenchResult r = bench_flat(counts[c], iterations);
            print_result(&r);
            free((char*)r.name);
            printf("\n");
        }
    }

    /* Section 2: All-types struct */
    printf("--- Section 2: All-Types Struct (16 fields) ---\n\n");
    {
        size_t counts[] = {100, 500};
        for (int c = 0; c < 2; c++) {
            BenchResult r = bench_all_types(counts[c], iterations);
            print_result(&r);
            free((char*)r.name);
            printf("\n");
        }
    }

    /* Section 3: Deep nesting */
    printf("--- Section 3: 5-Level Deep Nesting (Company hierarchy) ---\n\n");
    {
        size_t counts[] = {10, 50, 100};
        for (int c = 0; c < 3; c++) {
            BenchResult r = bench_deep(counts[c], iterations);
            print_result(&r);
            free((char*)r.name);
            printf("\n");
        }
    }

    /* Section 4: Single struct roundtrip */
    printf("--- Section 4: Single Struct Roundtrip (10000x) ---\n\n");
    {
        BUser user = {1, asun_string_from("Alice"), asun_string_from("alice@example.com"),
                      30, 95.5, true, asun_string_from("engineer"), asun_string_from("NYC")};

        double t0 = now_ms();
        for (int i = 0; i < 10000; i++) {
            asun_buf_t buf = asun_encode_BUser(&user);
            BUser r = {0};
            asun_decode_BUser(buf.data, buf.len, &r);
            asun_buf_free(&buf);
            free_buser(&r);
        }
        double asun_flat = now_ms() - t0;

        t0 = now_ms();
        for (int i = 0; i < 10000; i++) {
            asun_buf_t buf = asun_buf_new(256);
            json_serialize_user(&buf, &user);
            size_t n = 0;
            const char* p = buf.data;
            const char* e = buf.data + buf.len;
            BUser r = json_deserialize_user(&p, e);
            asun_buf_free(&buf);
            free_buser(&r);
        }
        double json_flat = now_ms() - t0;

        printf("  Flat:  ASUN %8.2fms | JSON %8.2fms | ratio %.2fx\n", asun_flat, json_flat, json_flat / asun_flat);
        free_buser(&user);
    }

    /* Section 5: Large payload */
    printf("\n--- Section 5: Large Payload (10k records) ---\n\n");
    {
        BenchResult r = bench_flat(10000, 10);
        printf("  (10 iterations for large payload)\n");
        print_result(&r);
        free((char*)r.name);
    }

    /* Section 6: Annotated vs Unannotated (serialize) */
    printf("\n--- Section 6: Annotated vs Unannotated Schema (serialize) ---\n\n");
    {
        size_t count = 1000;
        BUser* users = generate_users(count);
        int ser_iters = 200;

        double t0 = now_ms();
        asun_buf_t untyped_buf = {0};
        for (int i = 0; i < ser_iters; i++) {
            asun_buf_free(&untyped_buf);
            untyped_buf = asun_encode_vec_BUser(users, count);
        }
        double untyped_ms = now_ms() - t0;

        t0 = now_ms();
        asun_buf_t typed_buf = {0};
        for (int i = 0; i < ser_iters; i++) {
            asun_buf_free(&typed_buf);
            typed_buf = asun_encode_typed_vec_BUser(users, count);
        }
        double typed_ms = now_ms() - t0;

        printf("  Flat struct x 1000 (%d iters, serialize only)\n", ser_iters);
        printf("    Unannotated: %8.2fms  (%zu B)\n", untyped_ms, untyped_buf.len);
        printf("    Annotated:   %8.2fms  (%zu B)\n", typed_ms, typed_buf.len);
        printf("    Ratio: %.3fx (unannotated / annotated)\n", untyped_ms / typed_ms);

        asun_buf_free(&untyped_buf);
        asun_buf_free(&typed_buf);
        for (size_t i = 0; i < count; i++) free_buser(&users[i]);
        free(users);
    }

    /* Section 7: Annotated vs Unannotated (deserialize) */
    printf("\n--- Section 7: Annotated vs Unannotated Schema (deserialize) ---\n\n");
    {
        size_t count = 1000;
        BUser* users = generate_users(count);
        asun_buf_t untyped_buf = asun_encode_vec_BUser(users, count);
        asun_buf_t typed_buf = asun_encode_typed_vec_BUser(users, count);
        int de_iters = 200;

        double t0 = now_ms();
        for (int i = 0; i < de_iters; i++) {
            BUser* r = NULL; size_t n = 0;
            asun_decode_vec_BUser(untyped_buf.data, untyped_buf.len, &r, &n);
            for (size_t j = 0; j < n; j++) free_buser(&r[j]);
            free(r);
        }
        double untyped_ms = now_ms() - t0;

        t0 = now_ms();
        for (int i = 0; i < de_iters; i++) {
            BUser* r = NULL; size_t n = 0;
            asun_decode_vec_BUser(typed_buf.data, typed_buf.len, &r, &n);
            for (size_t j = 0; j < n; j++) free_buser(&r[j]);
            free(r);
        }
        double typed_ms = now_ms() - t0;

        printf("  Flat struct x 1000 (%d iters, deserialize only)\n", de_iters);
        printf("    Unannotated: %8.2fms  (%zu B)\n", untyped_ms, untyped_buf.len);
        printf("    Annotated:   %8.2fms  (%zu B)\n", typed_ms, typed_buf.len);
        printf("    Ratio: %.3fx (unannotated / annotated)\n", untyped_ms / typed_ms);

        asun_buf_free(&untyped_buf);
        asun_buf_free(&typed_buf);
        for (size_t i = 0; i < count; i++) free_buser(&users[i]);
        free(users);
    }

    /* Section 8: Throughput summary */
    printf("\n--- Section 8: Throughput Summary ---\n\n");
    {
        size_t count = 1000;
        BUser* users = generate_users(count);
        asun_buf_t json_buf = json_serialize_users(users, count);
        asun_buf_t asun_buf = asun_encode_vec_BUser(users, count);
        int iters = 100;

        double t0 = now_ms();
        for (int i = 0; i < iters; i++) { asun_buf_t b = json_serialize_users(users, count); asun_buf_free(&b); }
        double json_ser_dur = (now_ms() - t0) / 1000.0;

        t0 = now_ms();
        for (int i = 0; i < iters; i++) { asun_buf_t b = asun_encode_vec_BUser(users, count); asun_buf_free(&b); }
        double asun_ser_dur = (now_ms() - t0) / 1000.0;

        t0 = now_ms();
        for (int i = 0; i < iters; i++) {
            size_t n = 0; BUser* r = json_deserialize_users(json_buf.data, json_buf.len, &n);
            for (size_t j = 0; j < n; j++) free_buser(&r[j]); free(r);
        }
        double json_de_dur = (now_ms() - t0) / 1000.0;

        t0 = now_ms();
        for (int i = 0; i < iters; i++) {
            size_t n = 0; BUser* r = NULL;
            asun_decode_vec_BUser(asun_buf.data, asun_buf.len, &r, &n);
            for (size_t j = 0; j < n; j++) free_buser(&r[j]); free(r);
        }
        double asun_de_dur = (now_ms() - t0) / 1000.0;

        double total_records = (double)(count * (size_t)iters);
        double json_ser_rps = total_records / json_ser_dur;
        double asun_ser_rps = total_records / asun_ser_dur;
        double json_de_rps = total_records / json_de_dur;
        double asun_de_rps = total_records / asun_de_dur;

        double json_ser_mbps = (double)(json_buf.len * (size_t)iters) / json_ser_dur / 1048576.0;
        double asun_ser_mbps = (double)(asun_buf.len * (size_t)iters) / asun_ser_dur / 1048576.0;
        double json_de_mbps = (double)(json_buf.len * (size_t)iters) / json_de_dur / 1048576.0;
        double asun_de_mbps = (double)(asun_buf.len * (size_t)iters) / asun_de_dur / 1048576.0;

        printf("  Serialize throughput (1000 records x %d iters):\n", iters);
        printf("    JSON: %.0f records/s  (%.1f MB/s)\n", json_ser_rps, json_ser_mbps);
        printf("    ASUN: %.0f records/s  (%.1f MB/s)\n", asun_ser_rps, asun_ser_mbps);
        printf("    Speed: %.2fx%s\n", asun_ser_rps / json_ser_rps,
               asun_ser_rps > json_ser_rps ? " \xe2\x9c\x93 ASUN faster" : "");
        printf("  Deserialize throughput:\n");
        printf("    JSON: %.0f records/s  (%.1f MB/s)\n", json_de_rps, json_de_mbps);
        printf("    ASUN: %.0f records/s  (%.1f MB/s)\n", asun_de_rps, asun_de_mbps);
        printf("    Speed: %.2fx%s\n", asun_de_rps / json_de_rps,
               asun_de_rps > json_de_rps ? " \xe2\x9c\x93 ASUN faster" : "");

        asun_buf_free(&json_buf);
        asun_buf_free(&asun_buf);
        for (size_t i = 0; i < count; i++) free_buser(&users[i]);
        free(users);
    }

    printf("\nBenchmark Complete.\n");
    return 0;
}
