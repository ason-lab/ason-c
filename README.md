# ason-c

[![C11](https://img.shields.io/badge/C-11-blue.svg)](https://en.cppreference.com/w/c/11)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

High-performance C11 support for [ASON](https://github.com/ason-lab/ason), a schema-driven data format that removes repeated keys from structured payloads.

[中文文档](README_CN.md)

## Why ASON

ASON keeps the schema once and stores each row as a compact tuple:

```json
[
  {"id": 1, "name": "Alice", "active": true},
  {"id": 2, "name": "Bob", "active": false}
]
```

```text
[{id:int,name:str,active:bool}]:(1,Alice,true),(2,Bob,false)
```

That usually means fewer tokens, smaller payloads, and faster parsing than repeated-object JSON.

## Highlights

- Pure C11, no external dependencies
- SIMD-aware parser with scalar fallback
- Zero-copy-friendly text decoding
- Schema-driven text format and compact binary format
- Support for strings, numbers, bools, optional fields, vectors, maps, nested structs, and struct arrays

## Quick Start

Copy `include/ason.h` and `src/ason.c` into your project, then define a schema with the current macros:

```c
#include "ason.h"

typedef struct {
    int64_t id;
    ason_string_t name;
    bool active;
} User;

ASON_FIELDS(User, 3,
    ASON_FIELD(User, id,     "id",     i64),
    ASON_FIELD(User, name,   "name",   str),
    ASON_FIELD(User, active, "active", bool))
ASON_FIELDS_BIN(User, 3)
```

### Encode and decode a struct

```c
User user = {1, ason_string_from("Alice"), true};

ason_buf_t text = ason_encode_User(&user);
// {id,name,active}:(1,Alice,true)

ason_buf_t typed = ason_encode_typed_User(&user);
// {id:int,name:str,active:bool}:(1,Alice,true)

User decoded = {0};
ason_err_t err = ason_decode_User(text.data, text.len, &decoded);
assert(err == ASON_OK);

ason_buf_free(&text);
ason_buf_free(&typed);
ason_string_free(&user.name);
ason_string_free(&decoded.name);
```

### Encode and decode an array

```c
User users[2] = {
    {1, ason_string_from("Alice"), true},
    {2, ason_string_from("Bob"), false},
};

ason_buf_t text = ason_encode_vec_User(users, 2);
// [{id,name,active}]:(1,Alice,true),(2,Bob,false)

User *decoded = NULL;
size_t count = 0;
ason_err_t err = ason_decode_vec_User(text.data, text.len, &decoded, &count);
assert(err == ASON_OK && count == 2);
```

### Binary roundtrip

```c
ason_buf_t bin = ason_encode_bin_User(&user);

User decoded = {0};
ason_err_t err = ason_decode_bin_User(bin.data, bin.len, &decoded);
assert(err == ASON_OK);
```

## Current API

| Function family | Purpose |
| --- | --- |
| `ason_encode_T` / `ason_encode_typed_T` | Encode one struct to text |
| `ason_decode_T` | Decode one struct from text |
| `ason_encode_vec_T` / `ason_encode_typed_vec_T` | Encode struct arrays to text |
| `ason_decode_vec_T` | Decode struct arrays from text |
| `ason_encode_bin_T` / `ason_encode_bin_vec_T` | Encode to binary |
| `ason_decode_bin_T` / `ason_decode_bin_vec_T` | Decode from binary |

`T` is generated from your `ASON_FIELDS(...)` declaration.

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

- Flat 1,000-record dataset: ASON text serialize `37.91ms` vs JSON `49.94ms` and deserialize `89.72ms` vs JSON `264.03ms`
- Throughput summary: ASON text was `1.61x` faster than JSON for serialize and `1.86x` faster for deserialize
- Size summary for 1,000 flat records: JSON `121,675 B`, ASON text `56,718 B` (`53%` smaller), ASON binary `74,454 B` (`39%` smaller)
- Binary path was the fastest path in the benchmark: `6.31x` faster than JSON on flat 1,000-record serialization and `7.52x` faster on deserialization

For deeply nested 100-record company payloads, ASON text decoding was `3.10x` faster than JSON and ASON text size was `61%` smaller.

## Contributors

- [Athan](https://github.com/athxx)

## License

MIT
