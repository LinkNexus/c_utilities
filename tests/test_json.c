#include "../json.h"
#include "../arena.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Helper to test valid JSON
#define TEST_VALID(name, json_str) \
  do { \
    Arena arena = arena_create(4096); \
    JsonValue out = {0}; \
    char* err_msg = NULL; \
    bool result = json_parse(json_str, &arena, &out, &err_msg); \
    if (!result) { \
      printf("✗ " name " - Parse failed: %s\n", err_msg ? err_msg : "unknown error"); \
      assert(0); \
    } \
    arena_destroy(&arena); \
  } while(0)

// Helper to test invalid JSON
#define TEST_INVALID(name, json_str) \
  do { \
    Arena arena = arena_create(4096); \
    JsonValue out = {0}; \
    char* err_msg = NULL; \
    bool result = json_parse(json_str, &arena, &out, &err_msg); \
    if (result) { \
      printf("✗ " name " - Should have failed but didn't\n"); \
      assert(0); \
    } \
    arena_destroy(&arena); \
  } while(0)

// ============================================================================
// RFC 7158 COMPLIANCE TESTS - Spec-Compliant, Strict
// ============================================================================

void test_rfc_valid_primitives() {
  TEST_VALID("null", "null");
  TEST_VALID("true", "true");
  TEST_VALID("false", "false");
  printf("✓ test_rfc_valid_primitives\n");
}

void test_rfc_valid_numbers() {
  // Integer forms
  TEST_VALID("zero", "0");
  TEST_VALID("positive_int", "42");
  TEST_VALID("negative_int", "-123");
  
  // Decimal forms
  TEST_VALID("decimal", "3.14");
  TEST_VALID("negative_decimal", "-2.71");
  TEST_VALID("zero_decimal", "0.0");
  
  // Exponent forms (RFC 7158 section 6)
  TEST_VALID("exponent_e", "1e5");
  TEST_VALID("exponent_E", "1E5");
  TEST_VALID("exponent_positive", "1e+5");
  TEST_VALID("exponent_negative", "1e-5");
  TEST_VALID("exponent_decimal", "1.5e10");
  TEST_VALID("exponent_decimal_negative", "3.14e-2");
  
  printf("✓ test_rfc_valid_numbers\n");
}

void test_rfc_invalid_numbers() {
  // Leading zeros not allowed (RFC 7158)
  TEST_INVALID("leading_zero", "01");
  TEST_INVALID("leading_zero_decimal", "01.5");
  
  // Lone minus sign
  TEST_INVALID("lone_minus", "-");
  TEST_INVALID("lone_plus", "+5");
  
  // Decimal without digits
  TEST_INVALID("decimal_no_fraction", "5.");
  TEST_INVALID("decimal_no_integer", ".5");
  
  // Exponent without digits
  TEST_INVALID("exponent_no_digits", "1e");
  TEST_INVALID("exponent_no_digits_sign", "1e+");
  
  printf("✓ test_rfc_invalid_numbers\n");
}

void test_rfc_valid_strings() {
  // Empty string
  TEST_VALID("empty_string", "\"\"");
  
  // ASCII strings
  TEST_VALID("simple_string", "\"hello\"");
  TEST_VALID("string_with_spaces", "\"hello world\"");
  
  // Escape sequences (RFC 7158 section 7)
  TEST_VALID("escape_quote", "\"\\\"\"");
  TEST_VALID("escape_backslash", "\"\\\\\"");
  TEST_VALID("escape_forward_slash", "\"\\/\"");
  TEST_VALID("escape_backspace", "\"\\b\"");
  TEST_VALID("escape_formfeed", "\"\\f\"");
  TEST_VALID("escape_newline", "\"\\n\"");
  TEST_VALID("escape_carriage_return", "\"\\r\"");
  TEST_VALID("escape_tab", "\"\\t\"");
  
  // Unicode escapes
  TEST_VALID("unicode_ascii", "\"\\u0041\""); // 'A'
  TEST_VALID("unicode_latin", "\"\\u00E9\""); // 'é'
  TEST_VALID("unicode_cjk", "\"\\u4E2D\""); // '中'
  
  printf("✓ test_rfc_valid_strings\n");
}

void test_rfc_invalid_strings() {
  // Unescaped control characters
  TEST_INVALID("unescaped_newline", "\"line1\nline2\"");
  TEST_INVALID("unescaped_tab", "\"col1\tcol2\"");
  
  // Unterminated strings
  TEST_INVALID("unterminated", "\"hello");
  TEST_INVALID("unterminated_escape", "\"hello\\");
  
  // Invalid escape sequences
  TEST_INVALID("invalid_escape_x", "\"\\x41\"");
  TEST_INVALID("invalid_escape_u_incomplete", "\"\\u123\""); // needs 4 hex digits
  TEST_INVALID("invalid_escape_u_bad_hex", "\"\\uGGGG\"");
  
  printf("✓ test_rfc_invalid_strings\n");
}

void test_rfc_valid_arrays() {
  // Empty array
  TEST_VALID("empty_array", "[]");
  
  // Single element
  TEST_VALID("array_single_null", "[null]");
  TEST_VALID("array_single_bool", "[true]");
  TEST_VALID("array_single_number", "[42]");
  TEST_VALID("array_single_string", "[\"item\"]");
  TEST_VALID("array_single_array", "[[]]");
  TEST_VALID("array_single_object", "[{}]");
  
  // Multiple elements
  TEST_VALID("array_mixed_types", "[null, true, 42, \"string\", [], {}]");
  TEST_VALID("array_numbers", "[1, 2, 3, 4, 5]");
  TEST_VALID("array_strings", "[\"a\", \"b\", \"c\"]");
  
  // Nested arrays
  TEST_VALID("nested_arrays", "[[1, 2], [3, 4], [5, 6]]");
  TEST_VALID("deeply_nested_arrays", "[[[[[1]]]]]");
  
  // Whitespace (RFC 7158 section 2)
  TEST_VALID("array_with_spaces", "[ 1 , 2 , 3 ]");
  TEST_VALID("array_with_newlines", "[\n1,\n2,\n3\n]");
  TEST_VALID("array_with_tabs", "[\t1\t,\t2\t,\t3\t]");
  
  printf("✓ test_rfc_valid_arrays\n");
}

void test_rfc_invalid_arrays() {
  // Trailing comma
  TEST_INVALID("trailing_comma", "[1, 2,]");
  TEST_INVALID("trailing_comma_empty", "[,]");
  
  // Missing comma
  TEST_INVALID("missing_comma", "[1 2 3]");
  
  // Unclosed array
  TEST_INVALID("unclosed_array", "[1, 2, 3");
  
  // Invalid element
  TEST_INVALID("invalid_element", "[undefined]");
  TEST_INVALID("invalid_element_unquoted", "[hello]");
  
  printf("✓ test_rfc_invalid_arrays\n");
}

void test_rfc_valid_objects() {
  // Empty object
  TEST_VALID("empty_object", "{}");
  
  // Single property
  TEST_VALID("object_single_null", "{\"key\": null}");
  TEST_VALID("object_single_bool", "{\"key\": true}");
  TEST_VALID("object_single_number", "{\"key\": 42}");
  TEST_VALID("object_single_string", "{\"key\": \"value\"}");
  TEST_VALID("object_single_array", "{\"key\": []}");
  TEST_VALID("object_single_object", "{\"key\": {}}");
  
  // Multiple properties
  TEST_VALID("object_multiple", "{\"a\": 1, \"b\": 2, \"c\": 3}");
  
  // Nested objects
  TEST_VALID("object_nested", "{\"outer\": {\"inner\": {}}}");
  TEST_VALID("object_deeply_nested", "{\"a\": {\"b\": {\"c\": {\"d\": {}}}}}");
  
  // Complex structure
  TEST_VALID("object_complex", "{\"user\": {\"name\": \"Alice\", \"age\": 30, \"scores\": [10, 20, 30]}}");
  
  // Whitespace
  TEST_VALID("object_with_spaces", "{ \"key\" : \"value\" }");
  TEST_VALID("object_with_newlines", "{\n\"key\": \"value\"\n}");
  
  printf("✓ test_rfc_valid_objects\n");
}

void test_rfc_invalid_objects() {
  // Keys must be strings
  TEST_INVALID("object_unquoted_key", "{key: \"value\"}");
  TEST_INVALID("object_numeric_key", "{123: \"value\"}");
  
  // Missing colon
  TEST_INVALID("object_missing_colon", "{\"key\" \"value\"}");
  
  // Trailing comma
  TEST_INVALID("object_trailing_comma", "{\"a\": 1, \"b\": 2,}");
  TEST_INVALID("object_trailing_comma_single", "{\"key\": \"value\",}");
  
  // Missing comma
  TEST_INVALID("object_missing_comma", "{\"a\": 1 \"b\": 2}");
  
  // Unclosed object
  TEST_INVALID("object_unclosed", "{\"key\": \"value\"");
  
  // Invalid value
  TEST_INVALID("object_invalid_value", "{\"key\": undefined}");
  
  printf("✓ test_rfc_invalid_objects\n");
}

void test_rfc_top_level() {
  // RFC 7158 section 2: JSON text MUST NOT be empty
  TEST_INVALID("empty_input", "");
  TEST_INVALID("only_whitespace", "   ");
  TEST_INVALID("only_whitespace_newline", "\n\n\n");
  
  // Top-level values must be array or object in strict RFC 7158
  // However, RFC 7159 allows any JSON value at top level
  // We'll test what the implementation supports
  TEST_VALID("top_level_null", "null");
  TEST_VALID("top_level_bool", "true");
  TEST_VALID("top_level_number", "42");
  TEST_VALID("top_level_string", "\"hello\"");
  TEST_VALID("top_level_array", "[1, 2, 3]");
  TEST_VALID("top_level_object", "{\"key\": \"value\"}");
  
  printf("✓ test_rfc_top_level\n");
}

void test_rfc_extras() {
  // Extra data after valid JSON should fail
  TEST_INVALID("extra_data_after_value", "42 extra");
  TEST_INVALID("extra_data_after_array", "[] []");
  TEST_INVALID("extra_data_after_object", "{} {}");
  
  // Comments are not part of JSON spec (JSON5 extension)
  TEST_INVALID("comment_line", "// comment\n42");
  TEST_INVALID("comment_block", "/* comment */ 42");
  
  printf("✓ test_rfc_extras\n");
}

int main() {
  printf("=== RFC 7158 JSON Specification Compliance Tests ===\n\n");
  
  test_rfc_valid_primitives();
  test_rfc_valid_numbers();
  test_rfc_invalid_numbers();
  test_rfc_valid_strings();
  test_rfc_invalid_strings();
  test_rfc_valid_arrays();
  test_rfc_invalid_arrays();
  test_rfc_valid_objects();
  test_rfc_invalid_objects();
  test_rfc_top_level();
  test_rfc_extras();
  
  printf("\n✓ All compliance tests completed!\n");
  return 0;
}
