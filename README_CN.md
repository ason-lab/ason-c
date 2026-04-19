# asun-c

[![C11](https://img.shields.io/badge/C-11-blue.svg)](https://en.cppreference.com/w/c/11)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

面向 [ASUN](https://github.com/asunLab/asun) 的高性能 C11 实现。ASUN 是一种 Schema 驱动的数据格式，适合把重复对象压缩成更紧凑的结构化载荷。

[English](https://github.com/asunLab/asun-c/blob/main/README.md)

## 为什么用 ASUN

**json**

标准 JSON 会在每条记录里重复所有字段名。无论是发给 LLM、通过 API 传输，还是服务之间交换数据，这种重复都会浪费 Token、带宽和阅读成本：

```json
[
  { "id": 1, "name": "Alice", "active": true },
  { "id": 2, "name": "Bob", "active": false },
  { "id": 3, "name": "Carol", "active": true }
]
```

**asun**

ASUN 只声明 **一次** Schema，后续每一行只保留值：

```asun
[{id, name, active}]:
  (1,Alice,true),
  (2,Bob,false),
  (3,Carol,true)
```

**这通常意味着更少的 token、更小的体积，更清晰的结构, 以及比重复键名 JSON 更快的解析。**

---

## 特性

- 纯 C11，无第三方依赖
- SIMD 优化解析，带标量回退
- 文本解析尽量零拷贝
- 同时支持 ASUN 文本和紧凑二进制格式
- 支持字符串、数字、布尔、可选字段、数组、嵌套结构体、结构体数组
- 键值风格数据请通过普通结构体数组来建模，使用 `ASUN_FIELD_VEC_STRUCT(...)`

## 快速开始

把 `include/asun.h` 和 `src/asun.c` 放进项目后，先用当前宏定义 Schema：

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

### 编码和解码单个结构体

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

### 编码和解码结构体数组

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

### 二进制往返

```c
asun_buf_t bin = asun_encode_bin_User(&user);

User decoded = {0};
asun_err_t err = asun_decode_bin_User(bin.data, bin.len, &decoded);
assert(err == ASUN_OK);
```

## 当前 API

| 函数组                                          | 作用                 |
| ----------------------------------------------- | -------------------- |
| `asun_encode_T` / `asun_encode_typed_T`         | 编码单个结构体到文本 |
| `asun_decode_T`                                 | 从文本解码单个结构体 |
| `asun_encode_vec_T` / `asun_encode_typed_vec_T` | 编码结构体数组到文本 |
| `asun_decode_vec_T`                             | 从文本解码结构体数组 |
| `asun_encode_bin_T` / `asun_encode_bin_vec_T`   | 编码到二进制         |
| `asun_decode_bin_T` / `asun_decode_bin_vec_T`   | 从二进制解码         |

其中 `T` 来自你的 `ASUN_FIELDS(...)` 声明。

## 运行示例

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/basic
./build/complex_example
./build/bench
ctest --test-dir build
```

## 最新基准

在当前这台机器上通过下面命令实测：

```bash
./build/bench
```

关键结果：

- 扁平 1,000 条记录：ASUN 文本序列化 `37.91ms`，JSON `49.94ms`；反序列化 ASUN `89.72ms`，JSON `264.03ms`
- 吞吐总结：ASUN 文本序列化比 JSON 快 `1.61x`，反序列化快 `1.86x`
- 1,000 条扁平记录体积：JSON `121,675 B`，ASUN 文本 `56,718 B`（缩小 `53%`），ASUN 二进制 `74,454 B`（缩小 `39%`）
- 二进制路径是这轮测试里最快的：在 1,000 条扁平记录上，序列化比 JSON 快 `6.31x`，反序列化快 `7.52x`

对于 100 条五层嵌套 company 数据，ASUN 文本反序列化比 JSON 快 `3.10x`，文本体积缩小 `61%`。

## 说明

- `@int`、`@str` 这类终端标注是可省略的基本类型提示。
- 对复杂字段，结构标记不是可选的：嵌套对象和数组必须保留 `@{...}` 或 `@[...]`。

## Contributors

- [Athan](https://github.com/athxx)

## 许可证

MIT
