#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int _test_pass_count = 0;
static int _test_fail_count = 0;
static const char *_current_test = NULL;

#define TEST_SUITE(name)                                                       \
  do {                                                                         \
    _current_test = name;                                                      \
    printf("\n── %s ──\n", name);                                              \
  } while (0)

#define TEST_ASSERT(cond, fmt, ...)                                            \
  do {                                                                         \
    if (cond) {                                                                \
      printf("  [PASS] " fmt "\n", ##__VA_ARGS__);                             \
      _test_pass_count++;                                                      \
    } else {                                                                   \
      printf("  [FAIL] " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__); \
      _test_fail_count++;                                                      \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_FATAL(cond, fmt, ...)                                      \
  do {                                                                         \
    if (cond) {                                                                \
      printf("  [PASS] " fmt "\n", ##__VA_ARGS__);                             \
      _test_pass_count++;                                                      \
    } else {                                                                   \
      printf("  [FAIL] " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__); \
      _test_fail_count++;                                                      \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define TEST_SUMMARY()                                                         \
  do {                                                                         \
    printf("\n══════════════════════════════\n");                              \
    printf("  Results: %d passed, %d failed\n", _test_pass_count,              \
           _test_fail_count);                                                  \
    printf("══════════════════════════════\n");                                \
  } while (0)
