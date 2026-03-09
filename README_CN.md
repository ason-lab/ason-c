# ason-c

[![C11](https://img.shields.io/badge/C-11-blue.svg)](https://en.cppreference.com/w/c/11)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

面向 [ASON](https://github.com/ason-lab/ason) 的高性能 C11 实现。ASON 是一种 Schema 驱动的数据格式，适合把重复对象压缩成更紧凑的结构化载荷。

[English](README.md)

## 为什么用 ASON

ASON 只写一次 Schema，后续每一行只保留值：

```json
[
  {"id": 1, "name": "Alice", "active": true},
  {"id": 2, "name": "Bob", "active": false}
]
```

```text
[{id:int,name:str,active:bool}]:(1,Alice,true),(2,Bob,false)
```

这通常意味着更少的 token、更小的体积，以及比重复键名 JSON 更快的解析。

## 特性

- 纯 C11，无第三方依赖
- SIMD 优化解析，带标量回退
- 文本解析尽量零拷贝
- 同时支持 ASON 文本和紧凑二进制格式
- 支持字符串、数字、布尔、可选字段、向量、映射、嵌套结构体、结构体数组

## 快速开始

把 `include/ason.h` 和 `src/ason.c` 放进项目后，先用当前宏定义 Schema：

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

### 编码和解码单个结构体

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

### 编码和解码结构体数组

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

### 二进制往返

```c
ason_buf_t bin = ason_encode_bin_User(&user);

User decoded = {0};
ason_err_t err = ason_decode_bin_User(bin.data, bin.len, &decoded);
assert(err == ASON_OK);
```

## 当前 API

| 函数组 | 作用 |
| --- | --- |
| `ason_encode_T` / `ason_encode_typed_T` | 编码单个结构体到文本 |
| `ason_decode_T` | 从文本解码单个结构体 |
| `ason_encode_vec_T` / `ason_encode_typed_vec_T` | 编码结构体数组到文本 |
| `ason_decode_vec_T` | 从文本解码结构体数组 |
| `ason_encode_bin_T` / `ason_encode_bin_vec_T` | 编码到二进制 |
| `ason_decode_bin_T` / `ason_decode_bin_vec_T` | 从二进制解码 |

其中 `T` 来自你的 `ASON_FIELDS(...)` 声明。

## 运行示例

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/basic
./build/complex_example
./build/bench
ctest --test-dir build
```

## Latest Benchmarks

在当前这台机器上通过下面命令实测：

```bash
./build/bench
```

关键结果：

- 扁平 1,000 条记录：ASON 文本序列化 `37.91ms`，JSON `49.94ms`；反序列化 ASON `89.72ms`，JSON `264.03ms`
- 吞吐总结：ASON 文本序列化比 JSON 快 `1.61x`，反序列化快 `1.86x`
- 1,000 条扁平记录体积：JSON `121,675 B`，ASON 文本 `56,718 B`（缩小 `53%`），ASON 二进制 `74,454 B`（缩小 `39%`）
- 二进制路径是这轮测试里最快的：在 1,000 条扁平记录上，序列化比 JSON 快 `6.31x`，反序列化快 `7.52x`

对于 100 条五层嵌套 company 数据，ASON 文本反序列化比 JSON 快 `3.10x`，文本体积缩小 `61%`。

## Contributors

- [Athan](https://github.com/athxx)

## 许可证

MIT
