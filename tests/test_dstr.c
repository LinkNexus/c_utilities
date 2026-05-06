#include "../dstr.h"
#include "../test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_create() {
  TEST_SUITE("Create");

  Dstr s = dstr_create();

  TEST_ASSERT(s != NULL, "dstr_create must return non-NULL");
  TEST_ASSERT(DSTR_LEN(s) == 0, "length must be 0, got %zu", DSTR_LEN(s));
  TEST_ASSERT(DSTR_HDR(s)->cap == DSTR_MIN_CAP,
              "cap must be DSTR_MIN_CAP, got %zu", DSTR_HDR(s)->cap);
  TEST_ASSERT(s[0] == '\0', "buf must be null terminated");

  dstr_destroy(s);
}

void test_from() {
  TEST_SUITE("From");

  Dstr s = dstr_from("hello");

  TEST_ASSERT(s != NULL, "dstr_from must return non-NULL");
  TEST_ASSERT(DSTR_LEN(s) == 5, "length must be 5, got %zu", DSTR_LEN(s));
  TEST_ASSERT(strcmp(s, "hello") == 0, "content must be 'hello', got '%s'", s);
  TEST_ASSERT(s[5] == '\0', "must be null terminated");

  dstr_destroy(s);

  // empty string
  Dstr empty = dstr_from("");
  TEST_ASSERT(DSTR_LEN(empty) == 0, "empty dstr length must be 0, got %zu",
              DSTR_LEN(empty));
  TEST_ASSERT(empty[0] == '\0', "empty dstr must be null terminated");

  dstr_destroy(empty);
}

void test_from_strv() {
  TEST_SUITE("From strv");

  Strv sv = strv_from("hello world");
  Strv sub = strv_substr(sv, 0, 5);

  Dstr s = dstr_from_strv(sub);

  TEST_ASSERT(DSTR_LEN(s) == 5, "length must be 5, got %zu", DSTR_LEN(s));
  TEST_ASSERT(strcmp(s, "hello") == 0, "content must be 'hello', got '%s'", s);
  TEST_ASSERT(s[5] == '\0', "must be null terminated");

  dstr_destroy(s);
}

void test_append() {
  TEST_SUITE("Append");

  Dstr s = dstr_from("hello");
  dstr_append(s, " world");

  TEST_ASSERT(DSTR_LEN(s) == 11, "length must be 11, got %zu", DSTR_LEN(s));
  TEST_ASSERT(strcmp(s, "hello world") == 0,
              "content must be 'hello world', got '%s'", s);
  TEST_ASSERT(s[11] == '\0', "must be null terminated");

  // append empty string
  size_t len_before = DSTR_LEN(s);
  dstr_append(s, "");
  TEST_ASSERT(DSTR_LEN(s) == len_before,
              "append empty must not change length, got %zu", DSTR_LEN(s));

  // append triggering resize
  Dstr small = dstr_from("ab");
  dstr_append(small, "cdefghijklmnop");
  TEST_ASSERT(strcmp(small, "abcdefghijklmnop") == 0,
              "content must be correct after resize, got '%s'", small);
  TEST_ASSERT(DSTR_LEN(small) == 16,
              "length must be 16 after resize append, got %zu",
              DSTR_LEN(small));

  dstr_destroy(s);
  dstr_destroy(small);
}

void test_concat() {
  TEST_SUITE("Concat");

  Dstr a = dstr_from("hello");
  Dstr b = dstr_concat(a, " world");

  TEST_ASSERT(strcmp(b, "hello world") == 0,
              "concat result must be 'hello world', got '%s'", b);
  TEST_ASSERT(DSTR_LEN(b) == 11, "concat length must be 11, got %zu",
              DSTR_LEN(b));

  // original must be unmodified
  TEST_ASSERT(strcmp(a, "hello") == 0, "original must be unmodified, got '%s'",
              a);
  TEST_ASSERT(DSTR_LEN(a) == 5, "original length must be 5, got %zu",
              DSTR_LEN(a));

  dstr_destroy(a);
  dstr_destroy(b);
}

void test_fmt() {
  TEST_SUITE("Fmt");

  Dstr s = dstr_fmt("hello %s, you are %d years old", "world", 42);

  TEST_ASSERT(s != NULL, "dstr_fmt must return non-NULL");
  TEST_ASSERT(strcmp(s, "hello world, you are 42 years old") == 0,
              "fmt result must match, got '%s'", s);
  TEST_ASSERT(DSTR_LEN(s) == strlen("hello world, you are 42 years old"),
              "fmt length must match, got %zu", DSTR_LEN(s));
  TEST_ASSERT(s[DSTR_LEN(s)] == '\0', "must be null terminated");

  dstr_destroy(s);

  // empty format
  Dstr empty = dstr_fmt("");
  TEST_ASSERT(empty != NULL, "empty fmt must return non-NULL");
  TEST_ASSERT(DSTR_LEN(empty) == 0, "empty fmt length must be 0, got %zu",
              DSTR_LEN(empty));

  dstr_destroy(empty);

  // no format args
  Dstr plain = dstr_fmt("just a string");
  TEST_ASSERT(strcmp(plain, "just a string") == 0,
              "plain fmt must match, got '%s'", plain);

  dstr_destroy(plain);
}

void test_append_fmt() {
  TEST_SUITE("Append fmt");

  Dstr s = dstr_from("hello");
  dstr_append_fmt(s, " %s %d", "world", 42);

  TEST_ASSERT(strcmp(s, "hello world 42") == 0,
              "append_fmt result must be 'hello world 42', got '%s'", s);
  TEST_ASSERT(DSTR_LEN(s) == strlen("hello world 42"),
              "append_fmt length must match, got %zu", DSTR_LEN(s));
  TEST_ASSERT(s[DSTR_LEN(s)] == '\0', "must be null terminated");

  // append_fmt triggering resize
  Dstr small = dstr_from("x");
  dstr_append_fmt(small, "%s", "xxxxxxxxxxxxxxxxxx");
  TEST_ASSERT(DSTR_LEN(small) == 19, "length must be 19 after resize, got %zu",
              DSTR_LEN(small));
  TEST_ASSERT(small[19] == '\0', "must be null terminated after resize");

  dstr_destroy(s);
  dstr_destroy(small);
}

void test_concat_fmt() {
  TEST_SUITE("Concat fmt");

  Dstr a = dstr_from("hello");
  Dstr b = dstr_concat_fmt(a, " %s %d", "world", 42);

  TEST_ASSERT(strcmp(b, "hello world 42") == 0,
              "concat_fmt result must be 'hello world 42', got '%s'", b);
  TEST_ASSERT(DSTR_LEN(b) == strlen("hello world 42"),
              "concat_fmt length must match, got %zu", DSTR_LEN(b));

  // original must be unmodified
  TEST_ASSERT(strcmp(a, "hello") == 0,
              "original must be unmodified after concat_fmt, got '%s'", a);

  dstr_destroy(a);
  dstr_destroy(b);
}

void test_reserve() {
  TEST_SUITE("Reserve");

  Dstr s = dstr_from("hi");
  dstr_reserve(s, 64);

  TEST_ASSERT(DSTR_HDR(s)->cap == 64, "cap must be 64 after reserve, got %zu",
              DSTR_HDR(s)->cap);
  TEST_ASSERT(DSTR_LEN(s) == 2, "length must still be 2 after reserve, got %zu",
              DSTR_LEN(s));
  TEST_ASSERT(strcmp(s, "hi") == 0,
              "content must be preserved after reserve, got '%s'", s);

  dstr_destroy(s);
}

void test_to_cstr() {
  TEST_SUITE("To cstr");

  Dstr s = dstr_from("hello");
  char *c = dstr_to_cstr(s);

  TEST_ASSERT(strcmp(c, "hello") == 0, "cstr must match, got '%s'", c);
  TEST_ASSERT(c[5] == '\0', "cstr must be null terminated");

  // must be independent copy
  dstr_destroy(s);
  TEST_ASSERT(strcmp(c, "hello") == 0,
              "cstr must be independent of original dstr");

  free(c);
}

int main(void) {
  test_create();
  test_from();
  test_from_strv();
  test_append();
  test_concat();
  test_fmt();
  test_append_fmt();
  test_concat_fmt();
  test_reserve();
  test_to_cstr();

  TEST_SUMMARY();
  return _test_fail_count > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
