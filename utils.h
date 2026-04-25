#ifndef C_UTILS_UTILS
#define C_UTILS_UTILS

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(x) ((x) < 0 ? -(x) : (x))

#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdlib.h>

#define TEST_ASSERT(cond, fmt, ...)                                            \
  do {                                                                         \
    if (cond) {                                                                \
      printf("[PASS] " fmt "\n", ##__VA_ARGS__);                               \
    } else {                                                                   \
      printf("[FAIL] " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__);   \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *xrealloc(void *ptr, size_t new_size) {
  void *new_ptr = realloc(ptr, new_size);
  if (!new_ptr) {
    fprintf(stderr, "Memory reallocation failed\n");
    exit(EXIT_FAILURE);
  }
  return new_ptr;
}

void *xcalloc(size_t num, size_t size) {
  void *ptr = calloc(num, size);
  if (!ptr) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

#endif // !C_UTILS_UTILS
