#include "asun.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Struct definitions
 * ============================================================================
 */

/* Department / Employee */
typedef struct {
  asun_string_t title;
} Department;
ASUN_FIELDS(Department, 1, ASUN_FIELD(Department, title, "title", str))

typedef struct {
  int64_t id;
  asun_string_t name;
  Department dept;
  asun_vec_str skills;
  bool active;
} Employee;

ASUN_VEC_STRUCT_DEFINE(Department)

ASUN_FIELDS(Employee, 5, ASUN_FIELD(Employee, id, "id", i64),
            ASUN_FIELD(Employee, name, "name", str),
            ASUN_FIELD_STRUCT(Employee, dept, "dept", &Department_asun_desc),
            ASUN_FIELD(Employee, skills, "skills", vec_str),
            ASUN_FIELD(Employee, active, "active", bool))

/* Entry-list examples */
typedef struct {
  asun_string_t key;
  int64_t value;
} AttrEntry;
ASUN_FIELDS(AttrEntry, 2, ASUN_FIELD(AttrEntry, key, "key", str),
            ASUN_FIELD(AttrEntry, value, "value", i64))
ASUN_VEC_STRUCT_DEFINE(AttrEntry)

typedef struct {
  asun_string_t name;
  asun_vec_AttrEntry attrs;
} WithEntries;

ASUN_FIELDS(WithEntries, 2, ASUN_FIELD(WithEntries, name, "name", str),
            ASUN_FIELD_VEC_STRUCT(WithEntries, attrs, "attrs", AttrEntry))

/* Address / Nested */
typedef struct {
  asun_string_t city;
  int64_t zip;
} Address;
ASUN_FIELDS(Address, 2, ASUN_FIELD(Address, city, "city", str),
            ASUN_FIELD(Address, zip, "zip", i64))

typedef struct {
  asun_string_t name;
  Address addr;
} Nested;
ASUN_FIELDS(Nested, 2, ASUN_FIELD(Nested, name, "name", str),
            ASUN_FIELD_STRUCT(Nested, addr, "addr", &Address_asun_desc))

/* AllTypes */
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
  asun_vec_vec_i64 nested_vec;
} AllTypes;

ASUN_FIELDS(AllTypes, 17, ASUN_FIELD(AllTypes, b, "b", bool),
            ASUN_FIELD(AllTypes, i8v, "i8v", i8),
            ASUN_FIELD(AllTypes, i16v, "i16v", i16),
            ASUN_FIELD(AllTypes, i32v, "i32v", i32),
            ASUN_FIELD(AllTypes, i64v, "i64v", i64),
            ASUN_FIELD(AllTypes, u8v, "u8v", u8),
            ASUN_FIELD(AllTypes, u16v, "u16v", u16),
            ASUN_FIELD(AllTypes, u32v, "u32v", u32),
            ASUN_FIELD(AllTypes, u64v, "u64v", u64),
            ASUN_FIELD(AllTypes, f32v, "f32v", f32),
            ASUN_FIELD(AllTypes, f64v, "f64v", f64),
            ASUN_FIELD(AllTypes, s, "s", str),
            ASUN_FIELD(AllTypes, opt_some, "opt_some", opt_i64),
            ASUN_FIELD(AllTypes, opt_none, "opt_none", opt_i64),
            ASUN_FIELD(AllTypes, vec_int, "vec_int", vec_i64),
            ASUN_FIELD(AllTypes, vec_str, "vec_str", vec_str),
            ASUN_FIELD(AllTypes, nested_vec, "nested_vec", vec_vec_i64))

/* 5-level: Country > Region > City > District > Street > Building */
typedef struct {
  asun_string_t name;
  int64_t floors;
  bool residential;
  double height_m;
} Building;

ASUN_FIELDS(Building, 4, ASUN_FIELD(Building, name, "name", str),
            ASUN_FIELD(Building, floors, "floors", i64),
            ASUN_FIELD(Building, residential, "residential", bool),
            ASUN_FIELD(Building, height_m, "height_m", f64))
ASUN_VEC_STRUCT_DEFINE(Building)

typedef struct {
  asun_string_t name;
  double length_km;
  asun_vec_Building buildings;
} Street;

ASUN_FIELDS(Street, 3, ASUN_FIELD(Street, name, "name", str),
            ASUN_FIELD(Street, length_km, "length_km", f64),
            ASUN_FIELD_VEC_STRUCT(Street, buildings, "buildings", Building))
ASUN_VEC_STRUCT_DEFINE(Street)

typedef struct {
  asun_string_t name;
  int64_t population;
  asun_vec_Street streets;
} District;

ASUN_FIELDS(District, 3, ASUN_FIELD(District, name, "name", str),
            ASUN_FIELD(District, population, "population", i64),
            ASUN_FIELD_VEC_STRUCT(District, streets, "streets", Street))
ASUN_VEC_STRUCT_DEFINE(District)

typedef struct {
  asun_string_t name;
  int64_t population;
  double area_km2;
  asun_vec_District districts;
} City;

ASUN_FIELDS(City, 4, ASUN_FIELD(City, name, "name", str),
            ASUN_FIELD(City, population, "population", i64),
            ASUN_FIELD(City, area_km2, "area_km2", f64),
            ASUN_FIELD_VEC_STRUCT(City, districts, "districts", District))
ASUN_VEC_STRUCT_DEFINE(City)

typedef struct {
  asun_string_t name;
  asun_vec_City cities;
} Region;

ASUN_FIELDS(Region, 2, ASUN_FIELD(Region, name, "name", str),
            ASUN_FIELD_VEC_STRUCT(Region, cities, "cities", City))
ASUN_VEC_STRUCT_DEFINE(Region)

typedef struct {
  asun_string_t name;
  asun_string_t code;
  int64_t population;
  double gdp_trillion;
  asun_vec_Region regions;
} Country;

ASUN_FIELDS(Country, 5, ASUN_FIELD(Country, name, "name", str),
            ASUN_FIELD(Country, code, "code", str),
            ASUN_FIELD(Country, population, "population", i64),
            ASUN_FIELD(Country, gdp_trillion, "gdp_trillion", f64),
            ASUN_FIELD_VEC_STRUCT(Country, regions, "regions", Region))

/* Binary support for 5-level hierarchy */
ASUN_FIELDS_BIN(Building, 4)
ASUN_FIELDS_BIN(Street, 3)
ASUN_FIELDS_BIN(District, 3)
ASUN_FIELDS_BIN(City, 4)
ASUN_FIELDS_BIN(Region, 2)
ASUN_FIELDS_BIN(Country, 5)

/* 7-level: Universe > Galaxy > SolarSystem > Planet > Continent > Nation >
 * State */
typedef struct {
  asun_string_t name;
  asun_string_t capital;
  int64_t population;
} State;
ASUN_FIELDS(State, 3, ASUN_FIELD(State, name, "name", str),
            ASUN_FIELD(State, capital, "capital", str),
            ASUN_FIELD(State, population, "population", i64))
ASUN_VEC_STRUCT_DEFINE(State)

typedef struct {
  asun_string_t name;
  asun_vec_State states;
} Nation;
ASUN_FIELDS(Nation, 2, ASUN_FIELD(Nation, name, "name", str),
            ASUN_FIELD_VEC_STRUCT(Nation, states, "states", State))
ASUN_VEC_STRUCT_DEFINE(Nation)

typedef struct {
  asun_string_t name;
  asun_vec_Nation nations;
} Continent;
ASUN_FIELDS(Continent, 2, ASUN_FIELD(Continent, name, "name", str),
            ASUN_FIELD_VEC_STRUCT(Continent, nations, "nations", Nation))
ASUN_VEC_STRUCT_DEFINE(Continent)

typedef struct {
  asun_string_t name;
  double radius_km;
  bool has_life;
  asun_vec_Continent continents;
} Planet;
ASUN_FIELDS(Planet, 4, ASUN_FIELD(Planet, name, "name", str),
            ASUN_FIELD(Planet, radius_km, "radius_km", f64),
            ASUN_FIELD(Planet, has_life, "has_life", bool),
            ASUN_FIELD_VEC_STRUCT(Planet, continents, "continents", Continent))
ASUN_VEC_STRUCT_DEFINE(Planet)

typedef struct {
  asun_string_t name;
  asun_string_t star_type;
  asun_vec_Planet planets;
} SolarSystem;
ASUN_FIELDS(SolarSystem, 3, ASUN_FIELD(SolarSystem, name, "name", str),
            ASUN_FIELD(SolarSystem, star_type, "star_type", str),
            ASUN_FIELD_VEC_STRUCT(SolarSystem, planets, "planets", Planet))
ASUN_VEC_STRUCT_DEFINE(SolarSystem)

typedef struct {
  asun_string_t name;
  double star_count_billions;
  asun_vec_SolarSystem systems;
} Galaxy;
ASUN_FIELDS(Galaxy, 3, ASUN_FIELD(Galaxy, name, "name", str),
            ASUN_FIELD(Galaxy, star_count_billions, "star_count_billions", f64),
            ASUN_FIELD_VEC_STRUCT(Galaxy, systems, "systems", SolarSystem))
ASUN_VEC_STRUCT_DEFINE(Galaxy)

typedef struct {
  asun_string_t name;
  double age_billion_years;
  asun_vec_Galaxy galaxies;
} Universe;
ASUN_FIELDS(Universe, 3, ASUN_FIELD(Universe, name, "name", str),
            ASUN_FIELD(Universe, age_billion_years, "age_billion_years", f64),
            ASUN_FIELD_VEC_STRUCT(Universe, galaxies, "galaxies", Galaxy))

/* Binary support for 7-level hierarchy */
ASUN_FIELDS_BIN(State, 3)
ASUN_FIELDS_BIN(Nation, 2)
ASUN_FIELDS_BIN(Continent, 2)
ASUN_FIELDS_BIN(Planet, 4)
ASUN_FIELDS_BIN(SolarSystem, 3)
ASUN_FIELDS_BIN(Galaxy, 3)
ASUN_FIELDS_BIN(Universe, 3)

/* ServiceConfig */
typedef struct {
  asun_string_t host;
  int64_t port;
  int64_t max_connections;
  bool ssl;
  double timeout_ms;
} DbConfig;
ASUN_FIELDS(DbConfig, 5, ASUN_FIELD(DbConfig, host, "host", str),
            ASUN_FIELD(DbConfig, port, "port", i64),
            ASUN_FIELD(DbConfig, max_connections, "max_connections", i64),
            ASUN_FIELD(DbConfig, ssl, "ssl", bool),
            ASUN_FIELD(DbConfig, timeout_ms, "timeout_ms", f64))

typedef struct {
  bool enabled;
  int64_t ttl_seconds;
  int64_t max_size_mb;
} CacheConfig;
ASUN_FIELDS(CacheConfig, 3, ASUN_FIELD(CacheConfig, enabled, "enabled", bool),
            ASUN_FIELD(CacheConfig, ttl_seconds, "ttl_seconds", i64),
            ASUN_FIELD(CacheConfig, max_size_mb, "max_size_mb", i64))

typedef struct {
  asun_string_t level;
  asun_opt_str file;
  bool rotate;
} LogConfig;
ASUN_FIELDS(LogConfig, 3, ASUN_FIELD(LogConfig, level, "level", str),
            ASUN_FIELD(LogConfig, file, "file", opt_str),
            ASUN_FIELD(LogConfig, rotate, "rotate", bool))

typedef struct {
  asun_string_t key;
  asun_string_t value;
} EnvEntry;
ASUN_FIELDS(EnvEntry, 2, ASUN_FIELD(EnvEntry, key, "key", str),
            ASUN_FIELD(EnvEntry, value, "value", str))
ASUN_VEC_STRUCT_DEFINE(EnvEntry)

typedef struct {
  asun_string_t name;
  asun_string_t version;
  DbConfig db;
  CacheConfig cache;
  LogConfig log;
  asun_vec_str features;
  asun_vec_EnvEntry env;
} ServiceConfig;

ASUN_FIELDS(ServiceConfig, 7, ASUN_FIELD(ServiceConfig, name, "name", str),
            ASUN_FIELD(ServiceConfig, version, "version", str),
            ASUN_FIELD_STRUCT(ServiceConfig, db, "db", &DbConfig_asun_desc),
            ASUN_FIELD_STRUCT(ServiceConfig, cache, "cache",
                              &CacheConfig_asun_desc),
            ASUN_FIELD_STRUCT(ServiceConfig, log, "log", &LogConfig_asun_desc),
            ASUN_FIELD(ServiceConfig, features, "features", vec_str),
            ASUN_FIELD_VEC_STRUCT(ServiceConfig, env, "env", EnvEntry))

/* Note, Measurement, Nums, Special, Matrix3D, WithVec — for edge cases */
typedef struct {
  asun_string_t text;
} Note;
ASUN_FIELDS(Note, 1, ASUN_FIELD(Note, text, "text", str))

typedef struct {
  int64_t id;
  double value;
  asun_string_t label;
} Measurement;
ASUN_FIELDS(Measurement, 3, ASUN_FIELD(Measurement, id, "id", i64),
            ASUN_FIELD(Measurement, value, "value", f64),
            ASUN_FIELD(Measurement, label, "label", str))

typedef struct {
  int64_t a;
  double b;
  int64_t c;
} Nums;
ASUN_FIELDS(Nums, 3, ASUN_FIELD(Nums, a, "a", i64),
            ASUN_FIELD(Nums, b, "b", f64), ASUN_FIELD(Nums, c, "c", i64))

typedef struct {
  asun_string_t val;
} Special;
ASUN_FIELDS(Special, 1, ASUN_FIELD(Special, val, "val", str))

typedef struct {
  asun_vec_vec_i64 data;
} Matrix3D;
ASUN_FIELDS(Matrix3D, 1, ASUN_FIELD(Matrix3D, data, "data", vec_vec_i64))

typedef struct {
  asun_vec_i64 items;
} WithVec;
ASUN_FIELDS(WithVec, 1, ASUN_FIELD(WithVec, items, "items", vec_i64))

/* ============================================================================
 * Helper: free deeply nested structs
 * ============================================================================
 */

static void free_building(Building *b) { asun_string_free(&b->name); }
static void free_street(Street *s) {
  asun_string_free(&s->name);
  for (size_t i = 0; i < s->buildings.len; i++)
    free_building(&s->buildings.data[i]);
  asun_vec_Building_free(&s->buildings);
}
static void free_district(District *d) {
  asun_string_free(&d->name);
  for (size_t i = 0; i < d->streets.len; i++)
    free_street(&d->streets.data[i]);
  asun_vec_Street_free(&d->streets);
}
static void free_city(City *c) {
  asun_string_free(&c->name);
  for (size_t i = 0; i < c->districts.len; i++)
    free_district(&c->districts.data[i]);
  asun_vec_District_free(&c->districts);
}
static void free_region(Region *r) {
  asun_string_free(&r->name);
  for (size_t i = 0; i < r->cities.len; i++)
    free_city(&r->cities.data[i]);
  asun_vec_City_free(&r->cities);
}
static void free_country(Country *c) {
  asun_string_free(&c->name);
  asun_string_free(&c->code);
  for (size_t i = 0; i < c->regions.len; i++)
    free_region(&c->regions.data[i]);
  asun_vec_Region_free(&c->regions);
}

/* ============================================================================
 * Main
 * ============================================================================
 */

int main(void) {
  printf("=== ASUN C Complex Examples ===\n\n");
  int passed = 0;

  /* 1. Nested struct */
  printf("1. Nested struct:\n");
  {
    const char *input =
        "{id,name,dept@{title},skills@[],active}:(1,Alice,(Manager),[rust],true)";
    Employee emp = {0};
    asun_err_t err = asun_decode_Employee(input, strlen(input), &emp);
    assert(err == ASUN_OK);
    printf("   id=%lld name=%s dept=%s skills=[", (long long)emp.id,
           emp.name.data, emp.dept.title.data);
    for (size_t i = 0; i < emp.skills.len; i++) {
      if (i)
        printf(",");
      printf("%s", emp.skills.data[i].data);
    }
    printf("] active=%s\n\n", emp.active ? "true" : "false");
    asun_string_free(&emp.name);
    asun_string_free(&emp.dept.title);
    for (size_t i = 0; i < emp.skills.len; i++)
      asun_string_free(&emp.skills.data[i]);
    asun_vec_str_free(&emp.skills);
    passed++;
  }

  /* 2. Vec with nested structs */
  printf("2. Vec with nested structs:\n");
  {
    const char *input =
        "[{id,name,dept@{title},skills@[str],active}]:"
        "(1,Alice,(Manager),[Rust,Go],true),"
        "(2,Bob,(Engineer),[Python],false),"
        "(3,\"Carol Smith\",(Director),[Leadership,Strategy],true)";
    Employee *emps = NULL;
    size_t cnt = 0;
    asun_err_t err = asun_decode_vec_Employee(input, strlen(input), &emps, &cnt);
    assert(err == ASUN_OK);
    for (size_t i = 0; i < cnt; i++) {
      printf("   id=%lld name=%s dept=%s\n", (long long)emps[i].id,
             emps[i].name.data, emps[i].dept.title.data);
      asun_string_free(&emps[i].name);
      asun_string_free(&emps[i].dept.title);
      for (size_t j = 0; j < emps[i].skills.len; j++)
        asun_string_free(&emps[i].skills.data[j]);
      asun_vec_str_free(&emps[i].skills);
    }
    free(emps);
    printf("\n");
    passed++;
  }

  /* 3. Entry-list field */
  printf("3. Entry-list field:\n");
  {
    const char *input = "{name,attrs@[{key,value}]}:(Alice,[(age,30),(score,95)])";
    WithEntries item = {0};
    asun_err_t err = asun_decode_WithEntries(input, strlen(input), &item);
    assert(err == ASUN_OK);
    printf("   name=%s attrs=[", item.name.data);
    for (size_t i = 0; i < item.attrs.len; i++) {
      if (i)
        printf(",");
      printf("(%s,%lld)", item.attrs.data[i].key.data,
             (long long)item.attrs.data[i].value);
    }
    printf("]\n\n");
    asun_string_free(&item.name);
    for (size_t i = 0; i < item.attrs.len; i++)
      asun_string_free(&item.attrs.data[i].key);
    asun_vec_AttrEntry_free(&item.attrs);
    passed++;
  }

  /* 4. Nested struct roundtrip */
  printf("4. Nested struct roundtrip:\n");
  {
    Nested n = {asun_string_from("Alice"), {asun_string_from("NYC"), 10001}};
    asun_buf_t buf = asun_encode_Nested(&n);
    printf("   serialized:   %.*s\n", (int)buf.len, buf.data);
    Nested n2 = {0};
    asun_err_t err = asun_decode_Nested(buf.data, buf.len, &n2);
    assert(err == ASUN_OK);
    assert(strcmp(n2.name.data, "Alice") == 0);
    assert(strcmp(n2.addr.city.data, "NYC") == 0);
    assert(n2.addr.zip == 10001);
    printf("   roundtrip OK\n\n");
    asun_buf_free(&buf);
    asun_string_free(&n.name);
    asun_string_free(&n.addr.city);
    asun_string_free(&n2.name);
    asun_string_free(&n2.addr.city);
    passed++;
  }

  /* 5. Escaped strings */
  printf("5. Escaped strings:\n");
  {
    Note note = {asun_string_from("say \"hi\", then (wave)\tnewline\nend")};
    asun_buf_t buf = asun_encode_Note(&note);
    printf("   serialized:   %.*s\n", (int)buf.len, buf.data);
    Note note2 = {0};
    asun_err_t err = asun_decode_Note(buf.data, buf.len, &note2);
    assert(err == ASUN_OK);
    assert(strcmp(note.text.data, note2.text.data) == 0);
    printf("   escape roundtrip OK\n\n");
    asun_buf_free(&buf);
    asun_string_free(&note.text);
    asun_string_free(&note2.text);
    passed++;
  }

  /* 6. Float fields */
  printf("6. Float fields:\n");
  {
    Measurement m = {2, 95.0, asun_string_from("score")};
    asun_buf_t buf = asun_encode_Measurement(&m);
    printf("   serialized: %.*s\n", (int)buf.len, buf.data);
    Measurement m2 = {0};
    asun_err_t err = asun_decode_Measurement(buf.data, buf.len, &m2);
    assert(err == ASUN_OK);
    assert(m2.id == 2);
    assert(fabs(m2.value - 95.0) < 0.001);
    printf("   float roundtrip OK\n\n");
    asun_buf_free(&buf);
    asun_string_free(&m.label);
    asun_string_free(&m2.label);
    passed++;
  }

  /* 7. Negative numbers */
  printf("7. Negative numbers:\n");
  {
    Nums n = {-42, -3.15, -9223372036854775807LL};
    asun_buf_t buf = asun_encode_Nums(&n);
    printf("   serialized:   %.*s\n", (int)buf.len, buf.data);
    Nums n2 = {0};
    asun_err_t err = asun_decode_Nums(buf.data, buf.len, &n2);
    assert(err == ASUN_OK);
    assert(n2.a == -42);
    assert(n2.c == -9223372036854775807LL);
    printf("   negative roundtrip OK\n\n");
    asun_buf_free(&buf);
    passed++;
  }

  /* 8. All types struct */
  printf("8. All types struct:\n");
  {
    AllTypes all = {0};
    all.b = true;
    all.i8v = -128;
    all.i16v = -32768;
    all.i32v = -2147483647 - 1;
    all.i64v = -9223372036854775807LL;
    all.u8v = 255;
    all.u16v = 65535;
    all.u32v = 4294967295U;
    all.u64v = 18446744073709551615ULL;
    all.f32v = 3.15f;
    all.f64v = 2.718281828459045;
    all.s = asun_string_from("hello, world (test) [arr]");
    all.opt_some = (asun_opt_i64){true, 42};
    all.opt_none = (asun_opt_i64){false, 0};
    all.vec_int = asun_vec_i64_new();
    int64_t vi[] = {1, 2, 3, -4, 0};
    for (int i = 0; i < 5; i++)
      asun_vec_i64_push(&all.vec_int, vi[i]);
    all.vec_str = asun_vec_str_new();
    asun_vec_str_push(&all.vec_str, asun_string_from("alpha"));
    asun_vec_str_push(&all.vec_str, asun_string_from("beta gamma"));
    asun_vec_str_push(&all.vec_str, asun_string_from("delta"));
    all.nested_vec = asun_vec_vec_i64_new();
    asun_vec_i64 nv1 = asun_vec_i64_new();
    asun_vec_i64_push(&nv1, 1);
    asun_vec_i64_push(&nv1, 2);
    asun_vec_i64 nv2 = asun_vec_i64_new();
    asun_vec_i64_push(&nv2, 3);
    asun_vec_i64_push(&nv2, 4);
    asun_vec_i64_push(&nv2, 5);
    asun_vec_vec_i64_push(&all.nested_vec, nv1);
    asun_vec_vec_i64_push(&all.nested_vec, nv2);

    asun_buf_t buf = asun_encode_AllTypes(&all);
    printf("   serialized (%zu bytes):\n   %.*s\n", buf.len, (int)buf.len,
           buf.data);

    AllTypes all2 = {0};
    asun_err_t err = asun_decode_AllTypes(buf.data, buf.len, &all2);
    assert(err == ASUN_OK);
    assert(all2.b == true);
    assert(all2.i8v == -128);
    assert(all2.u64v == 18446744073709551615ULL);
    assert(all2.vec_int.len == 5);
    assert(all2.nested_vec.len == 2);
    printf("   all-types roundtrip OK\n\n");

    asun_buf_free(&buf);
    asun_string_free(&all.s);
    for (size_t i = 0; i < all.vec_str.len; i++)
      asun_string_free(&all.vec_str.data[i]);
    asun_vec_str_free(&all.vec_str);
    asun_vec_i64_free(&all.vec_int);
    for (size_t i = 0; i < all.nested_vec.len; i++)
      asun_vec_i64_free(&all.nested_vec.data[i]);
    asun_vec_vec_i64_free(&all.nested_vec);
    asun_string_free(&all2.s);
    for (size_t i = 0; i < all2.vec_str.len; i++)
      asun_string_free(&all2.vec_str.data[i]);
    asun_vec_str_free(&all2.vec_str);
    asun_vec_i64_free(&all2.vec_int);
    for (size_t i = 0; i < all2.nested_vec.len; i++)
      asun_vec_i64_free(&all2.nested_vec.data[i]);
    asun_vec_vec_i64_free(&all2.nested_vec);
    passed++;
  }

  /* 9. 5-level deep nesting */
  printf("9. Five-level nesting:\n");
  {
    Country country = {0};
    country.name = asun_string_from("Rustland");
    country.code = asun_string_from("RL");
    country.population = 50000000;
    country.gdp_trillion = 1.5;
    country.regions = asun_vec_Region_new();

    Region r1 = {asun_string_from("Northern"), asun_vec_City_new()};
    City c1 = {asun_string_from("Ferriton"), 2000000, 350.5,
               asun_vec_District_new()};
    District d1 = {asun_string_from("Downtown"), 500000, asun_vec_Street_new()};
    Street s1 = {asun_string_from("Main St"), 2.5, asun_vec_Building_new()};
    Building b1 = {asun_string_from("Tower A"), 50, false, 200.0};
    Building b2 = {asun_string_from("Apt Block 1"), 12, true, 40.5};
    asun_vec_Building_push(&s1.buildings, b1);
    asun_vec_Building_push(&s1.buildings, b2);
    Street s2 = {asun_string_from("Oak Ave"), 1.2, asun_vec_Building_new()};
    Building b3 = {asun_string_from("Library"), 3, false, 15.0};
    asun_vec_Building_push(&s2.buildings, b3);
    asun_vec_Street_push(&d1.streets, s1);
    asun_vec_Street_push(&d1.streets, s2);
    District d2 = {asun_string_from("Harbor"), 150000, asun_vec_Street_new()};
    Street s3 = {asun_string_from("Dock Rd"), 0.8, asun_vec_Building_new()};
    Building b4 = {asun_string_from("Warehouse 7"), 1, false, 8.0};
    asun_vec_Building_push(&s3.buildings, b4);
    asun_vec_Street_push(&d2.streets, s3);
    asun_vec_District_push(&c1.districts, d1);
    asun_vec_District_push(&c1.districts, d2);
    asun_vec_City_push(&r1.cities, c1);
    asun_vec_Region_push(&country.regions, r1);

    Region r2 = {asun_string_from("Southern"), asun_vec_City_new()};
    City c2 = {asun_string_from("Crabville"), 800000, 120.0,
               asun_vec_District_new()};
    District d3 = {asun_string_from("Old Town"), 200000, asun_vec_Street_new()};
    Street s4 = {asun_string_from("Heritage Ln"), 0.5, asun_vec_Building_new()};
    Building b5 = {asun_string_from("Museum"), 2, false, 12.0};
    Building b6 = {asun_string_from("Town Hall"), 4, false, 20.0};
    asun_vec_Building_push(&s4.buildings, b5);
    asun_vec_Building_push(&s4.buildings, b6);
    asun_vec_Street_push(&d3.streets, s4);
    asun_vec_District_push(&c2.districts, d3);
    asun_vec_City_push(&r2.cities, c2);
    asun_vec_Region_push(&country.regions, r2);

    asun_buf_t buf = asun_encode_Country(&country);
    printf("   serialized (%zu bytes)\n", buf.len);

    Country country2 = {0};
    asun_err_t err = asun_decode_Country(buf.data, buf.len, &country2);
    assert(err == ASUN_OK);
    assert(strcmp(country2.name.data, "Rustland") == 0);
    assert(country2.regions.len == 2);
    printf("   ✓ 5-level ASUN-text roundtrip OK\n");

    /* ASUN binary roundtrip */
    asun_buf_t bin = asun_encode_bin_Country(&country);
    Country country3 = {0};
    asun_err_t err2 = asun_decode_bin_Country(bin.data, bin.len, &country3);
    assert(err2 == ASUN_OK);
    assert(strcmp(country3.name.data, "Rustland") == 0);
    printf("   ✓ 5-level ASUN-bin roundtrip OK\n");
    printf("   ASUN text: %zu B | ASUN bin: %zu B\n", buf.len, bin.len);
    printf("   BIN is %.0f%% smaller than text\n\n",
           (1.0 - (double)bin.len / (double)buf.len) * 100.0);

    asun_buf_free(&buf);
    asun_buf_free(&bin);
    free_country(&country);
    free_country(&country2);
    free_country(&country3);
    passed++;
  }

  /* 10. 7-level deep */
  printf("10. Seven-level nesting:\n");
  {
    Universe u = {0};
    u.name = asun_string_from("Observable");
    u.age_billion_years = 13.8;
    u.galaxies = asun_vec_Galaxy_new();

    Galaxy g = {asun_string_from("Milky Way"), 250.0,
                asun_vec_SolarSystem_new()};
    SolarSystem ss = {asun_string_from("Sol"), asun_string_from("G2V"),
                      asun_vec_Planet_new()};

    Planet earth = {asun_string_from("Earth"), 6371.0, true,
                    asun_vec_Continent_new()};
    Continent asia = {asun_string_from("Asia"), asun_vec_Nation_new()};
    Nation japan = {asun_string_from("Japan"), asun_vec_State_new()};
    State tokyo = {asun_string_from("Tokyo"), asun_string_from("Shinjuku"),
                   14000000};
    State osaka = {asun_string_from("Osaka"), asun_string_from("Osaka City"),
                   8800000};
    asun_vec_State_push(&japan.states, tokyo);
    asun_vec_State_push(&japan.states, osaka);
    asun_vec_Nation_push(&asia.nations, japan);
    Nation china = {asun_string_from("China"), asun_vec_State_new()};
    State beijing = {asun_string_from("Beijing"), asun_string_from("Beijing"),
                     21500000};
    asun_vec_State_push(&china.states, beijing);
    asun_vec_Nation_push(&asia.nations, china);
    asun_vec_Continent_push(&earth.continents, asia);

    Continent europe = {asun_string_from("Europe"), asun_vec_Nation_new()};
    Nation germany = {asun_string_from("Germany"), asun_vec_State_new()};
    State bavaria = {asun_string_from("Bavaria"), asun_string_from("Munich"),
                     13000000};
    State berlin = {asun_string_from("Berlin"), asun_string_from("Berlin"),
                    3600000};
    asun_vec_State_push(&germany.states, bavaria);
    asun_vec_State_push(&germany.states, berlin);
    asun_vec_Nation_push(&europe.nations, germany);
    asun_vec_Continent_push(&earth.continents, europe);

    Planet mars = {asun_string_from("Mars"), 3389.5, false,
                   asun_vec_Continent_new()};

    asun_vec_Planet_push(&ss.planets, earth);
    asun_vec_Planet_push(&ss.planets, mars);
    asun_vec_SolarSystem_push(&g.systems, ss);
    asun_vec_Galaxy_push(&u.galaxies, g);

    asun_buf_t buf = asun_encode_Universe(&u);
    printf("   serialized (%zu bytes)\n", buf.len);

    Universe u2 = {0};
    asun_err_t err = asun_decode_Universe(buf.data, buf.len, &u2);
    assert(err == ASUN_OK);
    assert(strcmp(u2.name.data, "Observable") == 0);
    printf("   ✓ 7-level ASUN-text roundtrip OK\n");

    /* ASUN binary roundtrip */
    asun_buf_t bin = asun_encode_bin_Universe(&u);
    Universe u3 = {0};
    asun_err_t err2 = asun_decode_bin_Universe(bin.data, bin.len, &u3);
    assert(err2 == ASUN_OK);
    assert(strcmp(u3.name.data, "Observable") == 0);
    printf("   ✓ 7-level ASUN-bin roundtrip OK\n");
    printf("   ASUN text: %zu B | ASUN bin: %zu B\n", buf.len, bin.len);
    printf("   BIN is %.0f%% smaller than text\n\n",
           (1.0 - (double)bin.len / (double)buf.len) * 100.0);

    asun_buf_free(&buf);
    asun_buf_free(&bin);
    /* Note: deep free omitted for brevity — would need recursive free */
    passed++;
  }

  /* 11. Service config */
  printf("11. Complex config struct:\n");
  {
    ServiceConfig cfg = {0};
    cfg.name = asun_string_from("my-service");
    cfg.version = asun_string_from("2.1.0");
    cfg.db =
        (DbConfig){asun_string_from("db.example.com"), 5432, 100, true, 3000.5};
    cfg.cache = (CacheConfig){true, 3600, 512};
    cfg.log = (LogConfig){asun_string_from("info"),
                          {true, asun_string_from("/var/log/app.log")},
                          true};
    cfg.features = asun_vec_str_new();
    asun_vec_str_push(&cfg.features, asun_string_from("auth"));
    asun_vec_str_push(&cfg.features, asun_string_from("rate-limit"));
    asun_vec_str_push(&cfg.features, asun_string_from("websocket"));
    cfg.env = asun_vec_EnvEntry_new();
    asun_vec_EnvEntry_push(&cfg.env,
                           (EnvEntry){asun_string_from("RUST_LOG"),
                                      asun_string_from("debug")});
    asun_vec_EnvEntry_push(&cfg.env,
                           (EnvEntry){asun_string_from("DATABASE_URL"),
                                      asun_string_from("postgres://localhost:5432/mydb")});
    asun_vec_EnvEntry_push(&cfg.env,
                           (EnvEntry){asun_string_from("SECRET_KEY"),
                                      asun_string_from("abc123!@#")});

    asun_buf_t buf = asun_encode_ServiceConfig(&cfg);
    printf("   serialized (%zu bytes):\n   %.*s\n", buf.len,
           (int)(buf.len > 200 ? 200 : buf.len), buf.data);

    ServiceConfig cfg2 = {0};
    asun_err_t err = asun_decode_ServiceConfig(buf.data, buf.len, &cfg2);
    assert(err == ASUN_OK);
    assert(strcmp(cfg2.name.data, "my-service") == 0);
    assert(cfg2.db.port == 5432);
    printf("   config roundtrip OK\n\n");

    asun_buf_free(&buf);
    asun_string_free(&cfg.name);
    asun_string_free(&cfg.version);
    asun_string_free(&cfg.db.host);
    asun_string_free(&cfg.log.level);
    if (cfg.log.file.has_value)
      asun_string_free(&cfg.log.file.value);
    for (size_t i = 0; i < cfg.features.len; i++)
      asun_string_free(&cfg.features.data[i]);
    asun_vec_str_free(&cfg.features);
    for (size_t i = 0; i < cfg.env.len; i++) {
      asun_string_free(&cfg.env.data[i].key);
      asun_string_free(&cfg.env.data[i].value);
    }
    asun_vec_EnvEntry_free(&cfg.env);
    asun_string_free(&cfg2.name);
    asun_string_free(&cfg2.version);
    asun_string_free(&cfg2.db.host);
    asun_string_free(&cfg2.log.level);
    if (cfg2.log.file.has_value)
      asun_string_free(&cfg2.log.file.value);
    for (size_t i = 0; i < cfg2.features.len; i++)
      asun_string_free(&cfg2.features.data[i]);
    asun_vec_str_free(&cfg2.features);
    for (size_t i = 0; i < cfg2.env.len; i++) {
      asun_string_free(&cfg2.env.data[i].key);
      asun_string_free(&cfg2.env.data[i].value);
    }
    asun_vec_EnvEntry_free(&cfg2.env);
    passed++;
  }

  /* 12. Edge cases */
  printf("12. Edge cases:\n");
  {
    /* Empty vec */
    WithVec wv = {{0}};
    wv.items = asun_vec_i64_new();
    asun_buf_t buf = asun_encode_WithVec(&wv);
    printf("   empty vec: %.*s\n", (int)buf.len, buf.data);
    WithVec wv2 = {0};
    asun_err_t err = asun_decode_WithVec(buf.data, buf.len, &wv2);
    assert(err == ASUN_OK);
    assert(wv2.items.len == 0);
    asun_buf_free(&buf);
    asun_vec_i64_free(&wv.items);
    asun_vec_i64_free(&wv2.items);

    /* Special chars */
    Special sp = {
        asun_string_from("tabs\there, newlines\nhere, quotes\"and\\backslash")};
    buf = asun_encode_Special(&sp);
    printf("   special chars: %.*s\n", (int)buf.len, buf.data);
    Special sp2 = {0};
    err = asun_decode_Special(buf.data, buf.len, &sp2);
    assert(err == ASUN_OK);
    assert(strcmp(sp.val.data, sp2.val.data) == 0);
    asun_buf_free(&buf);
    asun_string_free(&sp.val);
    asun_string_free(&sp2.val);

    /* Bool-like string */
    Special sp3 = {asun_string_from("true")};
    buf = asun_encode_Special(&sp3);
    printf("   bool-like string: %.*s\n", (int)buf.len, buf.data);
    Special sp4 = {0};
    err = asun_decode_Special(buf.data, buf.len, &sp4);
    assert(err == ASUN_OK);
    assert(strcmp(sp4.val.data, "true") == 0);
    asun_buf_free(&buf);
    asun_string_free(&sp3.val);
    asun_string_free(&sp4.val);

    /* Number-like string */
    Special sp5 = {asun_string_from("12345")};
    buf = asun_encode_Special(&sp5);
    printf("   number-like string: %.*s\n", (int)buf.len, buf.data);
    Special sp6 = {0};
    err = asun_decode_Special(buf.data, buf.len, &sp6);
    assert(err == ASUN_OK);
    assert(strcmp(sp6.val.data, "12345") == 0);
    asun_buf_free(&buf);
    asun_string_free(&sp5.val);
    asun_string_free(&sp6.val);

    printf("   all edge cases OK\n\n");
    passed++;
  }

  /* 13. Triple-nested arrays */
  printf("13. Triple-nested arrays:\n");
  {
    Matrix3D m3 = {{0}};
    m3.data = asun_vec_vec_i64_new();
    asun_vec_i64 r1 = asun_vec_i64_new();
    asun_vec_i64_push(&r1, 1);
    asun_vec_i64_push(&r1, 2);
    asun_vec_i64 r2 = asun_vec_i64_new();
    asun_vec_i64_push(&r2, 3);
    asun_vec_i64_push(&r2, 4);
    asun_vec_i64 r3 = asun_vec_i64_new();
    asun_vec_i64_push(&r3, 5);
    asun_vec_i64_push(&r3, 6);
    asun_vec_i64_push(&r3, 7);
    asun_vec_vec_i64_push(&m3.data, r1);
    asun_vec_vec_i64_push(&m3.data, r2);
    asun_vec_vec_i64_push(&m3.data, r3);

    asun_buf_t buf = asun_encode_Matrix3D(&m3);
    printf("   %.*s\n", (int)buf.len, buf.data);
    Matrix3D m3b = {0};
    asun_err_t err = asun_decode_Matrix3D(buf.data, buf.len, &m3b);
    assert(err == ASUN_OK);
    assert(m3b.data.len == 3);
    assert(m3b.data.data[2].len == 3);
    printf("   triple-nested array roundtrip OK\n\n");

    asun_buf_free(&buf);
    for (size_t i = 0; i < m3.data.len; i++)
      asun_vec_i64_free(&m3.data.data[i]);
    asun_vec_vec_i64_free(&m3.data);
    for (size_t i = 0; i < m3b.data.len; i++)
      asun_vec_i64_free(&m3b.data.data[i]);
    asun_vec_vec_i64_free(&m3b.data);
    passed++;
  }

  /* 14. Comments */
  printf("14. Comments:\n");
  {
    const char *input =
        "/* Top-level */ {id,name,dept@{title},skills@[],active}:/* inline */ "
        "(1,Alice,(HR),[rust],true)";
    Employee emp = {0};
    asun_err_t err = asun_decode_Employee(input, strlen(input), &emp);
    assert(err == ASUN_OK);
    printf("   with inline comment: id=%lld name=%s dept=%s\n",
           (long long)emp.id, emp.name.data, emp.dept.title.data);
    printf("   comment parsing OK\n\n");
    asun_string_free(&emp.name);
    asun_string_free(&emp.dept.title);
    for (size_t i = 0; i < emp.skills.len; i++)
      asun_string_free(&emp.skills.data[i]);
    asun_vec_str_free(&emp.skills);
    passed++;
  }

  printf("\n=== All %d complex examples passed! ===\n", passed);
  return 0;
}
