#include "../json.h"
#include "../test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_literals_and_primitives() {
  TEST_SUITE("Literals and Primitives");

  Arena arena = arena_create(1024);
  JsonValue out = {0};
  char* err = NULL;

  // string
  TEST_ASSERT(json_parse("\"hello\"", &arena, &out, &err), "simple string must parse");
  TEST_ASSERT(out.type == JSON_STRING, "type must be JSON_STRING");
  TEST_ASSERT(strcmp(out.string_value, "hello") == 0, "string value must be 'hello', got '%s'",
              out.string_value);

  // number
  JsonValue out2 = {0};
  err = NULL;
  TEST_ASSERT(json_parse("-123.45e+1", &arena, &out2, &err), "number must parse");
  TEST_ASSERT(out2.type == JSON_NUMBER, "type must be JSON_NUMBER");
  TEST_ASSERT(out2.number_value == -123.45e+1, "number value must match expected, got %f",
              out2.number_value);

  // true/false/null
  JsonValue jtrue = {0}, jfalse = {0}, jnull = {0};
  err = NULL;
  TEST_ASSERT(json_parse("true", &arena, &jtrue, &err), "true must parse");
  TEST_ASSERT(jtrue.type == JSON_BOOL && jtrue.bool_value == true, "true must be boolean true");

  err = NULL;
  TEST_ASSERT(json_parse("false", &arena, &jfalse, &err), "false must parse");
  TEST_ASSERT(jfalse.type == JSON_BOOL && jfalse.bool_value == false,
              "false must be boolean false");

  err = NULL;
  TEST_ASSERT(json_parse("null", &arena, &jnull, &err), "null must parse");
  TEST_ASSERT(jnull.type == JSON_NULL, "null must produce JSON_NULL");

  arena_destroy(&arena);
}

void test_arrays_and_objects() {
  TEST_SUITE("Arrays and Objects");

  Arena arena = arena_create(2048);
  JsonValue out = {0};
  char* err = NULL;

  const char* input = "{\"a\": [1, 2, 3], \"b\": {\"c\": \"d\"}}";
  TEST_ASSERT(json_parse(input, &arena, &out, &err), "complex object must parse");
  TEST_ASSERT(out.type == JSON_OBJECT, "root must be object");

  // retrieve 'a' array
  JsonValue** ref = NULL;
  TEST_ASSERT(hashmap_get_ref(out.object_value, "a", &ref), "object must contain key 'a'");
  TEST_ASSERT((*ref)->type == JSON_ARRAY, "a must be an array");
  TEST_ASSERT(darr_len((*ref)->array_value) == 3, "array 'a' must have length 3");
  TEST_ASSERT((*ref)->array_value[0].type == JSON_NUMBER &&
                  (*ref)->array_value[0].number_value == 1,
              "first element must be number 1");

  // retrieve nested object b.c
  JsonValue** refb = NULL;
  TEST_ASSERT(hashmap_get_ref(out.object_value, "b", &refb), "object must contain key 'b'");
  TEST_ASSERT((*refb)->type == JSON_OBJECT, "b must be object");
  JsonValue** refc = NULL;
  TEST_ASSERT(hashmap_get_ref((*refb)->object_value, "c", &refc), "b must contain key 'c'");
  TEST_ASSERT((*refc)->type == JSON_STRING && strcmp((*refc)->string_value, "d") == 0,
              "b.c must be string 'd'");

  arena_destroy(&arena);
}

void test_string_escapes_and_unicode() {
  TEST_SUITE("String Escapes and Unicode");

  Arena arena = arena_create(1024);
  JsonValue out = {0};
  char* err = NULL;

  // escapes: JSON \n should decode to an actual newline
  if (json_parse("\"line\\nnext\"", &arena, &out, &err)) {
    TEST_ASSERT(out.type == JSON_STRING, "escaped value must be string");
    TEST_ASSERT(strcmp(out.string_value, "line\nnext") == 0,
                "escaped newline must be decoded into actual newline");
  } else {
    TEST_ASSERT(err != NULL, "err message must be provided on failure of escaped newline test");
  }

  // unicode escape (capital A) -> \u0041
  JsonValue u = {0};
  err = NULL;
  if (json_parse("\"\\u0041\"", &arena, &u, &err)) {
    TEST_ASSERT(u.type == JSON_STRING && strcmp(u.string_value, "A") == 0,
                "unicode escape must decode to 'A'");
  } else {
    TEST_ASSERT(err != NULL, "err message must be provided on failure of unicode test");
  }

  arena_destroy(&arena);
}

void test_errors() {
  TEST_SUITE("Errors (negative tests)");

  Arena arena = arena_create(512);
  JsonValue out = {0};
  char* err = NULL;

  TEST_ASSERT(!json_parse("\"unterminated", &arena, &out, &err),
              "unterminated string must fail to parse");
  TEST_ASSERT(err != NULL, "err message must be provided on failure");

  err = NULL;
  TEST_ASSERT(!json_parse("-", &arena, &out, &err), "lone '-' must fail to parse as number");
  TEST_ASSERT(err != NULL, "err message must be provided on invalid number");

  err = NULL;
  TEST_ASSERT(!json_parse("{\"a\": 1,}", &arena, &out, &err),
              "trailing comma in object must be invalid per spec");
  TEST_ASSERT(err != NULL, "err message must be provided for malformed object");

  arena_destroy(&arena);
}

void test_spec_edge_cases() {
  TEST_SUITE("Spec edge cases: leading zeros, trailing commas, extra text");

  Arena arena = arena_create(512);
  JsonValue out = {0};
  char* err = NULL;

  err = NULL;
  TEST_ASSERT(!json_parse("012", &arena, &out, &err),
              "numbers with leading zeros must be invalid per spec");
  TEST_ASSERT(err != NULL, "error message must be returned for leading zero number");

  err = NULL;
  TEST_ASSERT(json_parse("0", &arena, &out, &err), "single zero must parse");

  err = NULL;
  TEST_ASSERT(!json_parse("[1,]", &arena, &out, &err), "trailing comma in array must be invalid");
  TEST_ASSERT(err != NULL, "err message must be provided for trailing comma in array");

  err = NULL;
  TEST_ASSERT(!json_parse("\"ok\" garbage", &arena, &out, &err),
              "extra non-whitespace after JSON value must be invalid");
  TEST_ASSERT(err != NULL, "err message must be provided when extra text follows a value");

  err = NULL;
  TEST_ASSERT(!json_parse("true false", &arena, &out, &err),
              "multiple top-level values must be invalid per spec");
  TEST_ASSERT(err != NULL, "err message must be provided for multiple top-level values");

  arena_destroy(&arena);
}

void test_json_get_accessors() {
  TEST_SUITE("json_get and json_get_ref");

  Arena arena = arena_create(2048);
  JsonValue root = {0};
  char* err = NULL;

  const char* input = "{"
                      "\"name\": \"Alice\","
                      "\"age\": 30,"
                      "\"active\": true,"
                      "\"profile\": {\"city\": \"Paris\", \"scores\": [10, 20, 30]},"
                      "\"items\": [\"zero\", {\"label\": \"second\"}]"
                      "}";
  TEST_ASSERT(json_parse(input, &arena, &root, &err), "fixture JSON must parse");
  TEST_ASSERT(root.type == JSON_OBJECT, "fixture root must be an object");

  JsonValue* name_ref = NULL;
  TEST_ASSERT(json_get_ref(&root, "name", &name_ref), "json_get_ref must find object key");
  TEST_ASSERT(name_ref != NULL, "json_get_ref must populate out pointer");
  TEST_ASSERT(name_ref->type == JSON_STRING, "name must be a string");
  TEST_ASSERT(strcmp(name_ref->string_value, "Alice") == 0, "name must be 'Alice', got '%s'",
              name_ref->string_value);

  JsonValue name_copy = {0};
  TEST_ASSERT(json_get(&root, "name", &name_copy), "json_get must copy object value");
  TEST_ASSERT(name_copy.type == JSON_STRING, "json_get copy must preserve type");
  TEST_ASSERT(strcmp(name_copy.string_value, "Alice") == 0,
              "json_get copy must preserve string contents");

  JsonValue* score_ref = NULL;
  if (json_get_ref(&root, "profile.scores.1", &score_ref)) {
    TEST_ASSERT(score_ref->type == JSON_NUMBER && score_ref->number_value == 20,
                "profile.scores.1 must be 20");
  } else {
    TEST_ASSERT(false, "json_get_ref must follow nested object/array paths");
  }

  JsonValue score_copy = {0};
  if (json_get(&root, "profile.scores.2", &score_copy)) {
    TEST_ASSERT(score_copy.type == JSON_NUMBER && score_copy.number_value == 30,
                "profile.scores.2 must be 30");
  } else {
    TEST_ASSERT(false, "json_get must copy nested array value");
  }

  JsonValue* second_item_ref = NULL;
  if (json_get_ref(&root, "items.1.label", &second_item_ref)) {
    TEST_ASSERT(second_item_ref->type == JSON_STRING &&
                    strcmp(second_item_ref->string_value, "second") == 0,
                "items.1.label must be 'second'");
  } else {
    TEST_ASSERT(false, "json_get_ref must traverse into arrays and then objects");
  }

  // invalid traversal into a primitive must fail
  JsonValue* invalid_ref = (JsonValue*)0x1;
  TEST_ASSERT(!json_get_ref(&root, "age.value", &invalid_ref),
              "json_get_ref must fail when traversing beyond a primitive");
  TEST_ASSERT(invalid_ref == (JsonValue*)0x1,
              "json_get_ref should not overwrite output on failure");

  JsonValue invalid_copy = {0};
  TEST_ASSERT(!json_get(&root, "active.flag", &invalid_copy),
              "json_get must fail when traversing beyond a primitive");

  TEST_ASSERT(!json_get_ref(&root, "profile.scores.5", &invalid_ref),
              "json_get_ref must fail for out-of-bounds array index");
  TEST_ASSERT(!json_get_ref(&root, "profile.scores.one", &invalid_ref),
              "json_get_ref must fail for non-numeric array index");
  TEST_ASSERT(!json_get_ref(&root, "missing", &invalid_ref),
              "json_get_ref must fail for missing object keys");

  arena_destroy(&arena);
}

int main(void) {
  test_literals_and_primitives();
  test_arrays_and_objects();
  test_string_escapes_and_unicode();
  test_spec_edge_cases();
  test_json_get_accessors();
  test_errors();

  TEST_SUMMARY();
  return _test_fail_count > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
