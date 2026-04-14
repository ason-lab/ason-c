# asun-c

[![C11](https://img.shields.io/badge/C-11-blue.svg)](https://en.cppreference.com/w/c/11)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

High-performance C11 support for [ASUN](https://github.com/asun-lab/asun), a schema-driven data format that removes repeated keys from structured payloads.

[中文文档](README_CN.md)

## Why ASUN

ASUN keeps the schema once and stores each row as a compact tuple:

```json
[
  { "id": 1, "name": "Alice", "active": true },
  { "id": 2, "name": "Bob", "active": false }
]
```

```text
[{id@int,name@str,active@bool}]:(1,Alice,true),(2,Bob,false)
```

That usually means fewer tokens, smaller payloads, and faster parsing than repeated-object JSON.

## Highlights

- Pure C11, no external dependencies
- SIMD-aware parser with scalar fallback
- Zero-copy-friendly text decoding
- Schema-driven text format and compact binary format
- Support for strings, numbers, bools, optional fields, arrays, nested structs, and struct arrays
- Entry-list style data is modeled with ordinary structs plus `ASUN_FIELD_VEC_STRUCT(...)`

## Quick Start

Copy `include/asun.h` and `src/asun.c` into your project, then define a schema with the current macros:

```c
#include "asun.h"

typedef struct {
    int64_t id;
    asun_string_t name;
    bool active;
} User;

ASUN_FIELDS(User, 3,
    ASUN_FIELD(User, id,     "id",     i64),
    ASUN_FIELD(User, name,   "name",   str),
    ASUN_FIELD(User, active, "active", bool))
ASUN_FIELDS_BIN(User, 3)
```

### Encode and decode a struct

```c
User user = {1, asun_string_from("Alice"), true};

asun_buf_t text = asun_encode_User(&user);
// {id,name,active}:(1,Alice,true)

asun_buf_t typed = asun_encode_typed_User(&user);
// {id@int,name@str,active@bool}:(1,Alice,true)

User decoded = {0};
asun_err_t err = asun_decode_User(text.data, text.len, &decoded);
assert(err == ASUN_OK);

asun_buf_free(&text);
asun_buf_free(&typed);
asun_string_free(&user.name);
asun_string_free(&decoded.name);
```

### Encode and decode an array

```c
User users[2] = {
    {1, asun_string_from("Alice"), true},
    {2, asun_string_from("Bob"), false},
};

asun_buf_t text = asun_encode_vec_User(users, 2);
// [{id,name,active}]:(1,Alice,true),(2,Bob,false)

User *decoded = NULL;
size_t count = 0;
asun_err_t err = asun_decode_vec_User(text.data, text.len, &decoded, &count);
assert(err == ASUN_OK && count == 2);
```

### Binary roundtrip

```c
asun_buf_t bin = asun_encode_bin_User(&user);

User decoded = {0};
asun_err_t err = asun_decode_bin_User(bin.data, bin.len, &decoded);
assert(err == ASUN_OK);
```

## Current API

| Function family                                 | Purpose                        |
| ----------------------------------------------- | ------------------------------ |
| `asun_encode_T` / `asun_encode_typed_T`         | Encode one struct to text      |
| `asun_decode_T`                                 | Decode one struct from text    |
| `asun_encode_vec_T` / `asun_encode_typed_vec_T` | Encode struct arrays to text   |
| `asun_decode_vec_T`                             | Decode struct arrays from text |
| `asun_encode_bin_T` / `asun_encode_bin_vec_T`   | Encode to binary               |
| `asun_decode_bin_T` / `asun_decode_bin_vec_T`   | Decode from binary             |

`T` is generated from your `ASUN_FIELDS(...)` declaration.

## Run Examples

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/basic
./build/complex_example
./build/bench
ctest --test-dir build
```

## Latest Benchmarks

Measured on this machine with:

```bash
./build/bench
```

Headline numbers:

- Flat 1,000-record dataset: ASUN text serialize `37.91ms` vs JSON `49.94ms` and deserialize `89.72ms` vs JSON `264.03ms`
- Throughput summary: ASUN text was `1.61x` faster than JSON for serialize and `1.86x` faster for deserialize
- Size summary for 1,000 flat records: JSON `121,675 B`, ASUN text `56,718 B` (`53%` smaller), ASUN binary `74,454 B` (`39%` smaller)
- Binary path was the fastest path in the benchmark: `6.31x` faster than JSON on flat 1,000-record serialization and `7.52x` faster on deserialization

For deeply nested 100-record company payloads, ASUN text decoding was `3.10x` faster than JSON and ASUN text size was `61%` smaller.

## Notes

- `@` is the field binding marker in schema text.
- Scalar hints such as `@int` and `@str` are optional.
- Structural bindings for complex fields are not optional: nested objects and arrays must keep `@{...}` or `@[...]` in the schema.
- The C implementation is now aligned with the current spec and no longer supports the older dedicated map API.

## Contributors

- [Athan](https://github.com/athxx)

## License

MIT
