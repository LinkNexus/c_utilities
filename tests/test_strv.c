#include "../strv.h"
#include "../test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_construction()
{
  TEST_SUITE("Construction");

  Strv s = strv_from("hello");
  TEST_ASSERT(s.len == 5, "len must be 5, got %zu", s.len);
  TEST_ASSERT(s.buf != NULL, "buf must not be NULL");
  TEST_ASSERT(memcmp(s.buf, "hello", 5) == 0, "buf must match 'hello'");

  Strv s2 = strv_from_len("hello world", 5);
  TEST_ASSERT(s2.len == 5, "len must be 5, got %zu", s2.len);
  TEST_ASSERT(memcmp(s2.buf, "hello", 5) == 0, "buf must match 'hello'");

  Strv empty = strv_from("");
  TEST_ASSERT(empty.len == 0, "empty strv len must be 0, got %zu", empty.len);
}

void test_cmp_eq()
{
  TEST_SUITE("Compare / Equal");

  Strv a = strv_from("hello");
  Strv b = strv_from("hello");
  Strv c = strv_from("world");
  Strv d = strv_from("hell");
  Strv e = strv_from("helloo");

  TEST_ASSERT(strv_eq(a, b), "identical strings must be equal");
  TEST_ASSERT(!strv_eq(a, c), "different strings must not be equal");
  TEST_ASSERT(!strv_eq(a, d), "shorter string must not be equal");
  TEST_ASSERT(!strv_eq(a, e), "longer string must not be equal");

  TEST_ASSERT(strv_cmp(a, b) == 0, "cmp of equal strings must be 0");
  TEST_ASSERT(strv_cmp(a, c) < 0, "cmp 'hello' < 'world' must be negative");
  TEST_ASSERT(strv_cmp(c, a) > 0, "cmp 'world' > 'hello' must be positive");
  TEST_ASSERT(strv_cmp(a, d) > 0, "cmp 'hello' > 'hell' must be positive");
  TEST_ASSERT(strv_cmp(d, a) < 0, "cmp 'hell' < 'hello' must be negative");
}

void test_substr()
{
  TEST_SUITE("Substr");

  Strv s = strv_from("hello world");

  Strv sub = strv_substr(s, 6, 5);
  TEST_ASSERT(sub.len == 5, "substr len must be 5, got %zu", sub.len);
  TEST_ASSERT(strv_eq(sub, strv_from("world")), "substr must be 'world'");

  Strv sub2 = strv_substr(s, 0, 5);
  TEST_ASSERT(strv_eq(sub2, strv_from("hello")), "substr must be 'hello'");

  // start beyond len — clamps to empty
  Strv sub3 = strv_substr(s, 100, 5);
  TEST_ASSERT(sub3.len == 0, "substr beyond len must be empty, got %zu",
              sub3.len);

  // len beyond end — clamps to remaining
  Strv sub4 = strv_substr(s, 6, 100);
  TEST_ASSERT(strv_eq(sub4, strv_from("world")),
              "substr with overlong len must clamp to remaining");
}

void test_slice()
{
  TEST_SUITE("Slice");

  Strv s = strv_from("hello world");

  Strv sl = strv_slice(s, 6);
  TEST_ASSERT(strv_eq(sl, strv_from("world")), "slice from 6 must be 'world'");

  Strv sl2 = strv_slice(s, 0);
  TEST_ASSERT(strv_eq(sl2, s), "slice from 0 must equal original");

  Strv sl3 = strv_slice(s, 100);
  TEST_ASSERT(sl3.len == 0, "slice beyond len must be empty, got %zu", sl3.len);
}

void test_find()
{
  TEST_SUITE("Find");

  Strv s = strv_from("hello world");

  size_t idx = strv_find(s, strv_from("world"));
  TEST_ASSERT(idx == 6, "find 'world' must return 6, got %zu", idx);

  size_t idx2 = strv_find(s, strv_from("hello"));
  TEST_ASSERT(idx2 == 0, "find 'hello' must return 0, got %zu", idx2);

  size_t idx3 = strv_find(s, strv_from("xyz"));
  TEST_ASSERT(idx3 == (size_t)-1,
              "find missing substring must return SIZE_MAX");

  size_t idx4 = strv_find_char(s, 'w');
  TEST_ASSERT(idx4 == 6, "find_char 'w' must return 6, got %zu", idx4);

  size_t idx5 = strv_find_char(s, 'z');
  TEST_ASSERT(idx5 == (size_t)-1,
              "find_char missing char must return SIZE_MAX");
}

void test_rfind()
{
  TEST_SUITE("Rfind");

  Strv s = strv_from("hello hello");

  size_t idx = strv_rfind(s, strv_from("hello"));
  TEST_ASSERT(idx == 6,
              "rfind 'hello' must return last occurrence at 6, got %zu", idx);

  size_t idx2 = strv_rfind(s, strv_from("xyz"));
  TEST_ASSERT(idx2 == (size_t)-1,
              "rfind missing substring must return SIZE_MAX");
}

void test_starts_ends_with()
{
  TEST_SUITE("Starts/ends with");

  Strv s = strv_from("hello world");

  TEST_ASSERT(strv_starts_with(s, strv_from("hello")),
              "must start with 'hello'");
  TEST_ASSERT(!strv_starts_with(s, strv_from("world")),
              "must not start with 'world'");
  TEST_ASSERT(strv_starts_with(s, strv_from("")),
              "must start with empty string");
  TEST_ASSERT(!strv_starts_with(strv_from("hi"), strv_from("hello world")),
              "shorter string must not start with longer prefix");

  TEST_ASSERT(strv_ends_with(s, strv_from("world")), "must end with 'world'");
  TEST_ASSERT(!strv_ends_with(s, strv_from("hello")),
              "must not end with 'hello'");
  TEST_ASSERT(strv_ends_with(s, strv_from("")), "must end with empty string");
  TEST_ASSERT(!strv_ends_with(strv_from("hi"), strv_from("hello world")),
              "shorter string must not end with longer suffix");
}

void test_trim()
{
  TEST_SUITE("Trim");

  Strv s = strv_from("  hello  ");

  Strv tl = strv_trim_left(s);
  TEST_ASSERT(strv_eq(tl, strv_from("hello  ")),
              "trim_left must remove leading spaces");

  Strv tr = strv_trim_right(s);
  TEST_ASSERT(strv_eq(tr, strv_from("  hello")),
              "trim_right must remove trailing spaces");

  Strv t = strv_trim(s);
  TEST_ASSERT(strv_eq(t, strv_from("hello")), "trim must remove both sides");

  // tabs
  Strv tabs = strv_from("\t\thello\t");
  TEST_ASSERT(strv_eq(strv_trim(tabs), strv_from("hello")),
              "trim must handle tabs");

  // already trimmed
  Strv clean = strv_from("hello");
  TEST_ASSERT(strv_eq(strv_trim(clean), clean),
              "trim on clean string must be identity");

  // all spaces
  Strv spaces = strv_from("   ");
  TEST_ASSERT(strv_trim(spaces).len == 0,
              "trim of all spaces must be empty, got %zu",
              strv_trim(spaces).len);
}

void test_split()
{
  TEST_SUITE("Split");

  Strv s = strv_from("a,b,c,d");
  Strv *parts = strv_split(s, ',');

  TEST_ASSERT(darr_len(parts) == 4, "split must produce 4 parts, got %zu",
              darr_len(parts));
  TEST_ASSERT(strv_eq(parts[0], strv_from("a")), "parts[0] must be 'a'");
  TEST_ASSERT(strv_eq(parts[1], strv_from("b")), "parts[1] must be 'b'");
  TEST_ASSERT(strv_eq(parts[2], strv_from("c")), "parts[2] must be 'c'");
  TEST_ASSERT(strv_eq(parts[3], strv_from("d")), "parts[3] must be 'd'");

  // trailing delimiter
  Strv trailing = strv_from("a,b,");
  Strv *parts2 = strv_split(trailing, ',');
  TEST_ASSERT(darr_len(parts2) == 3,
              "trailing delim must produce 3 parts, got %zu", darr_len(parts2));
  TEST_ASSERT(parts2[2].len == 0,
              "last part must be empty after trailing delim");

  // no delimiter
  Strv no_delim = strv_from("hello");
  Strv *parts3 = strv_split(no_delim, ',');
  TEST_ASSERT(darr_len(parts3) == 1,
              "no delimiter must produce 1 part, got %zu", darr_len(parts3));
  TEST_ASSERT(strv_eq(parts3[0], strv_from("hello")),
              "single part must equal original");
}

void test_split_str()
{
  TEST_SUITE("Split str");

  Strv s = strv_from("a::b::c");
  Strv *parts = strv_split_str(s, strv_from("::"));

  TEST_ASSERT(darr_len(parts) == 3, "split_str must produce 3 parts, got %zu",
              darr_len(parts));
  TEST_ASSERT(strv_eq(parts[0], strv_from("a")), "parts[0] must be 'a'");
  TEST_ASSERT(strv_eq(parts[1], strv_from("b")), "parts[1] must be 'b'");
  TEST_ASSERT(strv_eq(parts[2], strv_from("c")), "parts[2] must be 'c'");

  // trailing delimiter
  Strv trailing = strv_from("a::b::");
  Strv *parts2 = strv_split_str(trailing, strv_from("::"));
  TEST_ASSERT(darr_len(parts2) == 3,
              "trailing delim must produce 3 parts, got %zu", darr_len(parts2));
  TEST_ASSERT(parts2[2].len == 0,
              "last part must be empty after trailing delim");

  // no delimiter
  Strv no_delim = strv_from("hello");
  Strv *parts3 = strv_split_str(no_delim, strv_from("::"));
  TEST_ASSERT(darr_len(parts3) == 1,
              "no delimiter must produce 1 part, got %zu", darr_len(parts3));
}

void test_split_any()
{
  TEST_SUITE("Split any");

  Strv s = strv_from("a,b;c.d");
  Strv *parts = strv_split_any(s, strv_from(",;."));

  TEST_ASSERT(darr_len(parts) == 4, "split_any must produce 4 parts, got %zu",
              darr_len(parts));
  TEST_ASSERT(strv_eq(parts[0], strv_from("a")), "parts[0] must be 'a'");
  TEST_ASSERT(strv_eq(parts[1], strv_from("b")), "parts[1] must be 'b'");
  TEST_ASSERT(strv_eq(parts[2], strv_from("c")), "parts[2] must be 'c'");
  TEST_ASSERT(strv_eq(parts[3], strv_from("d")), "parts[3] must be 'd'");

  // consecutive delimiters
  Strv consec = strv_from("a,,b");
  Strv *parts2 = strv_split_any(consec, strv_from(","));
  TEST_ASSERT(darr_len(parts2) == 3,
              "consecutive delimiters must produce empty part, got %zu",
              darr_len(parts2));
  TEST_ASSERT(parts2[1].len == 0,
              "middle part between consecutive delimiters must be empty");
}

void test_arena_split()
{
  TEST_SUITE("Arena split");

  Arena arena = arena_create(512);

  Strv s = strv_from("a,b,c");
  Strv *parts = arena_strv_split(&arena, s, ',');
  TEST_ASSERT(darr_len(parts) == 3, "arena_split must produce 3 parts, got %zu",
              darr_len(parts));
  TEST_ASSERT(strv_eq(parts[0], strv_from("a")), "parts[0] must be 'a'");
  TEST_ASSERT(strv_eq(parts[1], strv_from("b")), "parts[1] must be 'b'");
  TEST_ASSERT(strv_eq(parts[2], strv_from("c")), "parts[2] must be 'c'");

  Strv s2 = strv_from("a::b::");
  Strv *parts2 = arena_strv_split_str(&arena, s2, strv_from("::"));
  TEST_ASSERT(darr_len(parts2) == 3,
              "arena_split_str must produce 3 parts, got %zu", darr_len(parts2));
  TEST_ASSERT(parts2[2].len == 0, "last part must be empty after trailing delim");

  arena_destroy(&arena);
}

int main(void)
{
  test_construction();
  test_cmp_eq();
  test_substr();
  test_slice();
  test_find();
  test_rfind();
  test_starts_ends_with();
  test_trim();
  test_split();
  test_split_str();
  test_split_any();
  test_arena_split();

  TEST_SUMMARY();
  return _test_fail_count > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
