#ifndef C_UTILS_UTILS
#define C_UTILS_UTILS

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(x) ((x) < 0 ? -(x) : (x))

static inline void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

static inline void *xrealloc(void *ptr, size_t new_size) {
  void *new_ptr = realloc(ptr, new_size);
  if (!new_ptr) {
    fprintf(stderr, "Memory reallocation failed\n");
    exit(EXIT_FAILURE);
  }
  return new_ptr;
}

static inline void *xcalloc(size_t num, size_t size) {
  void *ptr = calloc(num, size);
  if (!ptr) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

#define print_fn_err_msg(fmt, ...)                                             \
  (fprintf(stderr, "Error at (%s:%d): " fmt "\n", __FILE__, __LINE__,          \
           ##__VA_ARGS__))

#endif // !C_UTILS_UTILS
