#include "asun.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Generation structures
// ----------------------------------------------------------------------------
typedef struct {
  int64_t id;
  asun_string_t name;
  int32_t age;
  bool gender;
} Detail;

ASUN_FIELDS(Detail, 4, ASUN_FIELD(Detail, id, "ID", i64),
            ASUN_FIELD(Detail, name, "Name", str),
            ASUN_FIELD(Detail, age, "Age", i32),
            ASUN_FIELD(Detail, gender, "Gender", bool))

ASUN_VEC_STRUCT_DEFINE(Detail)

typedef struct {
  asun_vec_Detail details;
} User;

ASUN_FIELDS(User, 1, ASUN_FIELD_VEC_STRUCT(User, details, "details", Detail))

// ----------------------------------------------------------------------------
// Consumption structures
// ----------------------------------------------------------------------------
typedef struct {
  int64_t id;
  asun_string_t name;
  int32_t age;
} Person;

ASUN_FIELDS(Person, 3, ASUN_FIELD(Person, id, "ID", i64),
            ASUN_FIELD(Person, name, "Name", str),
            ASUN_FIELD(Person, age, "Age", i32))

ASUN_VEC_STRUCT_DEFINE(Person)

typedef struct {
  asun_vec_Person details;
} Human;

ASUN_FIELDS(Human, 1, ASUN_FIELD_VEC_STRUCT(Human, details, "details", Person))

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------
int main(void) {
  // 1. Setup User data
  User u = {0};
  u.details = asun_vec_Detail_new();

  Detail d1 = {1, asun_string_from("Alice"), 30, true};
  Detail d2 = {2, asun_string_from("Bob"), 25, false};

  asun_vec_Detail_push(&u.details, d1);
  asun_vec_Detail_push(&u.details, d2);

  User users[1] = {u};

  // 2. Encode
  asun_buf_t buf = asun_encode_vec_User(users, 1);
  printf("Encoded ASUN:\n%.*s\n", (int)buf.len, buf.data);

  // 3. Decode into Human list
  Human *humans = NULL;
  size_t count = 0;
  asun_err_t err = asun_decode_vec_Human(buf.data, buf.len, &humans, &count);
  assert(err == ASUN_OK);

  printf("\nDecoded into Human list:\n");
  for (size_t i = 0; i < count; i++) {
    printf("Human{details=[");
    for (size_t j = 0; j < humans[i].details.len; j++) {
      if (j > 0)
        printf(", ");
      printf("Person{ID=%lld, Name=\"%s\", Age=%d}",
             (long long)humans[i].details.data[j].id,
             humans[i].details.data[j].name.data,
             humans[i].details.data[j].age);
    }
    printf("]}\n");
  }

  // Cleanup
  asun_buf_free(&buf);
  for (size_t i = 0; i < count; i++) {
    for (size_t j = 0; j < humans[i].details.len; j++) {
      asun_string_free(&humans[i].details.data[j].name);
    }
    asun_vec_Person_free(&humans[i].details);
  }
  free(humans);

  asun_string_free(&d1.name);
  asun_string_free(&d2.name);
  asun_vec_Detail_free(&u.details);

  return 0;
}
