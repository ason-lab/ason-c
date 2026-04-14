#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "asun.h"

/* ============================================================================
 * Struct definitions
 * ============================================================================ */

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

typedef struct {
    int64_t id;
    asun_opt_str label;
} Item;

ASUN_FIELDS(Item, 2,
    ASUN_FIELD(Item, id,    "id",    i64),
    ASUN_FIELD(Item, label, "label", opt_str))

typedef struct {
    asun_string_t name;
    asun_vec_str tags;
} Tagged;

ASUN_FIELDS(Tagged, 2,
    ASUN_FIELD(Tagged, name, "name", str),
    ASUN_FIELD(Tagged, tags, "tags", vec_str))

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("=== ASUN C Basic Examples ===\n\n");

    /* 1. Serialize a single struct */
    printf("1. Serialize single struct:\n");
    User user = {1, asun_string_from("Alice"), true};
    asun_buf_t buf = asun_encode_User(&user);
    printf("   %.*s\n\n", (int)buf.len, buf.data);
    asun_buf_free(&buf);

    /* 2. Serialize with type annotations */
    printf("2. Serialize with type annotations:\n");
    buf = asun_encode_typed_User(&user);
    printf("   %.*s\n\n", (int)buf.len, buf.data);
    assert(buf.len > 0);
    asun_buf_free(&buf);

    /* 3. Deserialize from ASUN */
    printf("3. Deserialize single struct:\n");
    {
        const char* input = "{id@int,name@str,active@bool}:(1,Alice,true)";
        User u2 = {0};
        asun_err_t err = asun_decode_User(input, strlen(input), &u2);
        assert(err == ASUN_OK);
        printf("   id=%lld name=%s active=%s\n\n",
               (long long)u2.id, u2.name.data, u2.active ? "true" : "false");
        asun_string_free(&u2.name);
    }

    /* 4. Serialize a vec of structs */
    printf("4. Serialize vec (schema-driven):\n");
    {
        User users[3] = {
            {1, asun_string_from("Alice"), true},
            {2, asun_string_from("Bob"), false},
            {3, asun_string_from("Carol Smith"), true},
        };
        buf = asun_encode_vec_User(users, 3);
        printf("   %.*s\n\n", (int)buf.len, buf.data);
        asun_buf_free(&buf);
        for (int i = 0; i < 3; i++) asun_string_free(&users[i].name);
    }

    /* 5. Deserialize vec */
    printf("5. Deserialize vec:\n");
    {
        const char* input = "[{id@int,name@str,active@bool}]:(1,Alice,true),(2,Bob,false),(3,\"Carol Smith\",true)";
        User* users = NULL;
        size_t count = 0;
        asun_err_t err = asun_decode_vec_User(input, strlen(input), &users, &count);
        assert(err == ASUN_OK);
        for (size_t i = 0; i < count; i++) {
            printf("   id=%lld name=%s active=%s\n",
                   (long long)users[i].id, users[i].name.data,
                   users[i].active ? "true" : "false");
            asun_string_free(&users[i].name);
        }
        free(users);
        printf("\n");
    }

    /* 6. Multiline format */
    printf("6. Multiline format:\n");
    {
        const char* input = "[{id, name, active}]:\n  (1, Alice, true),\n  (2, Bob, false),\n  (3, \"Carol Smith\", true)";
        User* users = NULL;
        size_t count = 0;
        asun_err_t err = asun_decode_vec_User(input, strlen(input), &users, &count);
        assert(err == ASUN_OK);
        for (size_t i = 0; i < count; i++) {
            printf("   id=%lld name=%s active=%s\n",
                   (long long)users[i].id, users[i].name.data,
                   users[i].active ? "true" : "false");
            asun_string_free(&users[i].name);
        }
        free(users);
        printf("\n");
    }

    /* 7. Roundtrip (ASUN-text + ASUN-bin) */
    printf("7. Roundtrip (ASUN-text vs ASUN-bin):\n");
    {
        User orig = {42, asun_string_from("Test User"), true};
        /* ASUN text roundtrip */
        buf = asun_encode_User(&orig);
        User decoded = {0};
        asun_err_t err = asun_decode_User(buf.data, buf.len, &decoded);
        assert(err == ASUN_OK);
        assert(decoded.id == 42);
        assert(strcmp(decoded.name.data, "Test User") == 0);
        assert(decoded.active == true);
        printf("   ASUN text:    %.*s (%zu B)\n", (int)buf.len, buf.data, buf.len);
        asun_string_free(&decoded.name);

        /* ASUN binary roundtrip */
        asun_buf_t bin = asun_encode_bin_User(&orig);
        User decoded_bin = {0};
        err = asun_decode_bin_User(bin.data, bin.len, &decoded_bin);
        assert(err == ASUN_OK);
        assert(decoded_bin.id == 42);
        assert(strcmp(decoded_bin.name.data, "Test User") == 0);
        assert(decoded_bin.active == true);
        printf("   ASUN binary:  %zu B\n", bin.len);
        printf("   BIN is %.0f%% smaller than text\n",
               (1.0 - (double)bin.len / (double)buf.len) * 100.0);
        printf("   \xe2\x9c\x93 both formats roundtrip OK\n\n");
        asun_buf_free(&buf);
        asun_buf_free(&bin);
        asun_string_free(&orig.name);
        asun_string_free(&decoded_bin.name);
    }

    /* 8. Optional fields */
    printf("8. Optional fields:\n");
    {
        const char* input1 = "{id,label}:(1,hello)";
        Item item1 = {0};
        asun_err_t err = asun_decode_Item(input1, strlen(input1), &item1);
        assert(err == ASUN_OK);
        printf("   with value: id=%lld label=%s\n",
               (long long)item1.id, item1.label.has_value ? item1.label.value.data : "(null)");
        if (item1.label.has_value) asun_string_free(&item1.label.value);

        const char* input2 = "{id,label}:(2,)";
        Item item2 = {0};
        err = asun_decode_Item(input2, strlen(input2), &item2);
        assert(err == ASUN_OK);
        assert(!item2.label.has_value);
        printf("   with null:  id=%lld label=%s\n\n",
               (long long)item2.id, item2.label.has_value ? item2.label.value.data : "(null)");
    }

    /* 9. Array fields */
    printf("9. Array fields:\n");
    {
        const char* input = "{name,tags@[]}:(Alice,[rust,go,python])";
        Tagged t = {0};
        asun_err_t err = asun_decode_Tagged(input, strlen(input), &t);
        assert(err == ASUN_OK);
        printf("   name=%s tags=[", t.name.data);
        for (size_t i = 0; i < t.tags.len; i++) {
            if (i > 0) printf(",");
            printf("%s", t.tags.data[i].data);
        }
        printf("]\n\n");
        asun_string_free(&t.name);
        for (size_t i = 0; i < t.tags.len; i++) asun_string_free(&t.tags.data[i]);
        asun_vec_str_free(&t.tags);
    }

    /* 10. Comments */
    printf("10. With comments:\n");
    {
        const char* input = "/* user list */ {id,name,active}:(1,Alice,true)";
        User u = {0};
        asun_err_t err = asun_decode_User(input, strlen(input), &u);
        assert(err == ASUN_OK);
        printf("   id=%lld name=%s active=%s\n\n",
               (long long)u.id, u.name.data, u.active ? "true" : "false");
        asun_string_free(&u.name);
    }

    printf("=== All examples passed! ===\n");
    asun_string_free(&user.name);
    return 0;
}
