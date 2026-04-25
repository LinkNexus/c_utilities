#ifndef C_UTILS_DYNAMIC_ARRAYS
#define C_UTILS_DYNAMIC_ARRAYS

#include "utils.h"
#define MIN_DYNAMIC_ARRAY_SIZE 4

#define VEC_FIELDS(type)                                                       \
  type *items;                                                                 \
  size_t capacity;                                                             \
  size_t size;                                                                 \
  void (*destructor)(type * val);

#define VEC_OF(type) {VEC_FIELDS(type)}

#define VEC_EL_SIZE(vec) (sizeof *(vec)->items)

#define vec_destroy(vec)                                                       \
  do {                                                                         \
    if ((vec)->destructor) {                                                   \
      for (size_t i = 0; i < (vec)->size; ++i) {                               \
        (vec)->destructor(&((vec)->items[i]));                                 \
      }                                                                        \
    }                                                                          \
    free((vec)->items);                                                        \
    (vec)->items = NULL;                                                       \
    (vec)->size = 0;                                                           \
    (vec)->capacity = 0;                                                       \
  } while (0)

#define vec_reserve(vec, min_cap)                                              \
  do {                                                                         \
    if ((vec)->capacity < (min_cap)) {                                         \
      size_t new_cap = MAX((vec)->capacity, (min_cap));                        \
                                                                               \
      if (new_cap > SIZE_MAX / sizeof(*(vec)->items)) {                        \
        fprintf(stderr, "vec_reserve: Error: capacity overflow\n");            \
        break;                                                                 \
      }                                                                        \
                                                                               \
      (vec)->items = xrealloc((vec)->items, new_cap * sizeof(*(vec)->items));  \
      (vec)->capacity = new_cap;                                               \
    }                                                                          \
  } while (0)

#define vec_ensure_cap(vec, min_cap)                                           \
  do {                                                                         \
    if ((vec)->capacity >= (min_cap))                                          \
      break;                                                                   \
                                                                               \
    size_t new_capacity =                                                      \
        MAX(MAX((vec)->capacity * 2, (min_cap)), MIN_DYNAMIC_ARRAY_SIZE);      \
                                                                               \
    vec_reserve((vec), new_capacity);                                          \
  } while (0);

#define vec_append(vec, el)                                                    \
  do {                                                                         \
    vec_ensure_cap((vec), (vec)->size + 1);                                    \
    memcpy((vec)->items + (vec)->size, el, sizeof(*(vec)->items));             \
    (vec)->size += 1;                                                          \
  } while (0);

#define vec_iterate_with_idx(vec, fn)                                          \
  do {                                                                         \
    for (size_t i = 0; i < (vec)->size; ++i) {                                 \
      fn(&((vec)->items[i]), i);                                               \
    }                                                                          \
  } while (0);

#define vec_iterate(vec, fn)                                                   \
  do {                                                                         \
    for (size_t i = 0; i < (vec)->size; ++i) {                                 \
      fn(&((vec)->items[i]));                                                  \
    }                                                                          \
  } while (0);

#define vec_adjust_cap(vec)                                                    \
  do {                                                                         \
    if ((vec)->size < (vec)->capacity / 4) {                                   \
      size_t new_cap = MAX((vec)->capacity / 2, MIN_DYNAMIC_ARRAY_SIZE);       \
      (vec)->items = xrealloc((vec)->items, new_cap * sizeof *(vec)->items);   \
      (vec)->capacity = new_cap;                                               \
    }                                                                          \
  } while (0);

#define vec_pop(vec)                                                           \
  do {                                                                         \
    if ((vec)->size == 0)                                                      \
      break;                                                                   \
                                                                               \
    (vec)->size = (vec)->size - 1;                                             \
    if ((vec)->destructor)                                                     \
      (vec)->destructor(&((vec)->items[(vec)->size]));                         \
                                                                               \
    vec_adjust_cap((vec));                                                     \
  } while (0);

#define vec_set(vec, idx, el)                                                  \
  do {                                                                         \
    if (idx < 0 || idx >= (vec)->size)                                         \
      fprintf(stderr, "vec_set: Index out of bounds\n");                       \
    memcpy(&(vec)->items[idx], el, sizeof *(vec)->items);                      \
  } while (0);

#define vec_insert_at(vec, idx, el)                                            \
  do {                                                                         \
    if ((idx) < 0 || (idx) > (vec)->size) {                                    \
      fprintf(stderr, "vec_insert_at: Index out of bounds\n");                 \
      break;                                                                   \
    }                                                                          \
                                                                               \
    vec_ensure_cap((vec), (vec)->size + 1);                                    \
                                                                               \
    memmove(&((vec)->items[(idx) + 1]), &((vec)->items[(idx)]),                \
            ((vec)->size - (idx)) * sizeof *(vec)->items);                     \
    memcpy(&((vec)->items[(idx)]), (el), sizeof *(vec)->items);                \
                                                                               \
    (vec)->size += 1;                                                          \
  } while (0);

#define vec_remove_at(vec, idx)                                                \
  do {                                                                         \
    if ((idx) < 0 || (idx) >= (vec)->size) {                                   \
      fprintf(stderr, "vec_remove_at: Index out of bounds\n");                 \
      break;                                                                   \
    }                                                                          \
                                                                               \
    if ((vec)->destructor)                                                     \
      (vec)->destructor(&(vec)->items[(idx)]);                                 \
                                                                               \
    memmove(&((vec)->items[(idx)]), &((vec)->items[(idx) + 1]),                \
            ((vec)->size - 1) * sizeof *(vec)->items);                         \
                                                                               \
    (vec)->size -= 1;                                                          \
                                                                               \
    vec_adjust_cap((vec));                                                     \
  } while (0);

#define vec_append_range(vec, elements, count)                                 \
  do {                                                                         \
    vec_ensure_cap((vec), (vec)->size + (count));                              \
                                                                               \
    memcpy(&(vec)->items[(vec)->size], (elements),                             \
           (count) * VEC_EL_SIZE((vec)));                                      \
                                                                               \
    (vec)->size += count;                                                      \
  } while (0);

#define vec_clone(vec, target)                                                 \
  do {                                                                         \
    size_t total_size = (vec)->size * VEC_EL_SIZE((vec));                      \
                                                                               \
    (target)->items = xmalloc((vec)->capacity * VEC_EL_SIZE((vec)));           \
    memcpy((target)->items, (vec)->items, total_size);                         \
    (target)->size = (vec)->size;                                              \
    (target)->capacity = (vec)->capacity;                                      \
  } while (0);

#define vec_filter(vec, target, fn)                                            \
  do {                                                                         \
    for (size_t i = 0; i < (vec)->size; ++i) {                                 \
      if (fn(&(vec)->items[i])) {                                              \
        vec_append((target), &(vec)->items[i]);                                \
      }                                                                        \
    }                                                                          \
  } while (0);

#endif // !C_UTILS_DYNAMIC_ARRAYS
