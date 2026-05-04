#ifndef C_UTILS_DYNAMIC_ARRAYS
#define C_UTILS_DYNAMIC_ARRAYS

#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MIN_DYNAMIC_ARRAY_SIZE 4

typedef struct {
  size_t cap;
  size_t size;
  void (*destructor)(void *val);
} DarrHdr;

#define darr_print_err_msg(fmt, ...)                                           \
  (fprintf(stderr, "Error at (%s:%d): " fmt "\n", __FILE__, __LINE__,          \
           ##__VA_ARGS__))

#define DARR_HDR(darr) ((DarrHdr *)(darr) - 1)

#define darr_len(darr) (DARR_HDR(darr)->size)
#define darr_cap(darr) (DARR_HDR(darr)->cap)

#define darr_init(darr)                                                        \
  do {                                                                         \
    if (darr)                                                                  \
      break;                                                                   \
    DarrHdr *hdr = (DarrHdr *)xmalloc(                                         \
        MIN_DYNAMIC_ARRAY_SIZE * sizeof *(darr) + sizeof *hdr);                \
    hdr->size = 0;                                                             \
    hdr->cap = MIN_DYNAMIC_ARRAY_SIZE;                                         \
    hdr->destructor = NULL;                                                    \
                                                                               \
    (darr) = (typeof(darr))(hdr + 1);                                          \
  } while (0);

#define darr_set_destructor(darr, fn)                                          \
  ((DARR_HDR(darr))->destructor = (void (*)(void *))(fn))

#define darr_reserve(darr, new_cap)                                            \
  do {                                                                         \
    DarrHdr *hdr = DARR_HDR(darr);                                             \
                                                                               \
    if (new_cap < hdr->size) {                                                 \
      darr_print_err_msg("darr_reserve: new_cap smaller than current size");   \
      break;                                                                   \
    }                                                                          \
                                                                               \
    if (new_cap > SIZE_MAX / sizeof *(darr)) {                                 \
      darr_print_err_msg("darr_reserve: Capacity overflow");                   \
      break;                                                                   \
    }                                                                          \
                                                                               \
    hdr = (DarrHdr *)xrealloc(hdr, new_cap * sizeof(*(darr)) + sizeof *hdr);   \
    hdr->cap = new_cap;                                                        \
    (darr) = (typeof(darr))(hdr + 1);                                          \
  } while (0);

#define darr_ensure_cap(darr, count)                                           \
  do {                                                                         \
    darr_init(darr);                                                           \
    DarrHdr *hdr = DARR_HDR(darr);                                             \
    if (hdr->size + count <= hdr->cap)                                         \
      break;                                                                   \
    size_t new_cap = MAX(hdr->cap * 2, hdr->cap + count);                      \
    darr_reserve(darr, new_cap);                                               \
  } while (0);

#define darr_append(darr, el)                                                  \
  do {                                                                         \
    darr_ensure_cap(darr, 1);                                                  \
    DarrHdr *hdr = DARR_HDR(darr);                                             \
    (darr)[hdr->size] = el;                                                    \
    hdr->size++;                                                               \
  } while (0);

#define darr_adjust_cap(darr)                                                  \
  do {                                                                         \
    DarrHdr *hdr = DARR_HDR(darr);                                             \
                                                                               \
    if (hdr->size >= hdr->cap / 4)                                             \
      break;                                                                   \
                                                                               \
    size_t new_cap = MAX(hdr->cap / 2, MIN_DYNAMIC_ARRAY_SIZE);                \
    hdr = xrealloc(hdr, new_cap * sizeof(*(darr)) + sizeof *hdr);              \
    hdr->cap = new_cap;                                                        \
    (darr) = (void *)(hdr + 1);                                                \
  } while (0);

#define darr_pop(darr)                                                         \
  do {                                                                         \
    DarrHdr *hdr = DARR_HDR(darr);                                             \
                                                                               \
    if (!hdr->size)                                                            \
      break;                                                                   \
                                                                               \
    hdr->size--;                                                               \
                                                                               \
    if (hdr->destructor)                                                       \
      hdr->destructor(&(darr)[hdr->size]);                                     \
                                                                               \
    darr_adjust_cap(darr);                                                     \
  } while (0);

#define darr_set(darr, idx, el)                                                \
  do {                                                                         \
    DarrHdr *hdr = DARR_HDR(darr);                                             \
                                                                               \
    if ((idx) > hdr->size) {                                                   \
      darr_print_err_msg("darr_set: Index %zu out of bounds", (size_t)(idx));  \
      break;                                                                   \
    }                                                                          \
                                                                               \
    if ((idx) == hdr->size) {                                                  \
      darr_ensure_cap(darr, 1);                                                \
      DARR_HDR(darr)->size++;                                                  \
    }                                                                          \
                                                                               \
    (darr)[idx] = el;                                                          \
  } while (0);

#define darr_insert_at(darr, idx, el)                                          \
  do {                                                                         \
    DarrHdr *hdr = DARR_HDR(darr);                                             \
                                                                               \
    if ((idx) > hdr->size) {                                                   \
      darr_print_err_msg("darr_insert_at: Index %zu out of bounds",            \
                         (size_t)(idx));                                       \
      break;                                                                   \
    }                                                                          \
                                                                               \
    darr_ensure_cap(darr, 1);                                                  \
    hdr = DARR_HDR(darr);                                                      \
                                                                               \
    memmove((darr) + ((idx) + 1), (darr) + (idx),                              \
            (hdr->size - (idx)) * sizeof *(darr));                             \
    (darr)[idx] = el;                                                          \
    hdr->size++;                                                               \
  } while (0);

#define darr_remove_at(darr, idx)                                              \
  do {                                                                         \
    DarrHdr *hdr = DARR_HDR(darr);                                             \
                                                                               \
    if ((idx) >= hdr->size) {                                                  \
      darr_print_err_msg("darr_remove_at: Index %zu out of bounds",            \
                         (size_t)(idx));                                       \
      break;                                                                   \
    }                                                                          \
                                                                               \
    if (hdr->destructor)                                                       \
      hdr->destructor(&(darr)[idx]);                                           \
                                                                               \
    memmove((darr) + (idx), (darr) + ((idx) + 1),                              \
            (hdr->size - (idx) - 1) * sizeof *(darr));                         \
    hdr->size--;                                                               \
    darr_adjust_cap(darr);                                                     \
  } while (0);

#define darr_append_range(darr, elements, count)                               \
  do {                                                                         \
    darr_ensure_cap(darr, count);                                              \
    DarrHdr *hdr = DARR_HDR(darr);                                             \
                                                                               \
    memcpy((darr) + hdr->size, elements, count * sizeof *(darr));              \
    hdr->size += count;                                                        \
  } while (0);

#define darr_clone(src, dst)                                                   \
  do {                                                                         \
    DarrHdr *src_hdr = DARR_HDR(src);                                          \
    DarrHdr *dst_hdr =                                                         \
        xmalloc(src_hdr->cap * sizeof *(src) + sizeof *dst_hdr);               \
                                                                               \
    memcpy(dst_hdr + 1, src_hdr + 1, src_hdr->size * sizeof *(src));           \
                                                                               \
    dst_hdr->size = src_hdr->size;                                             \
    dst_hdr->cap = src_hdr->cap;                                               \
    dst_hdr->destructor = src_hdr->destructor;                                 \
    (dst) = (void *)(dst_hdr + 1);                                             \
  } while (0);

#define darr_destroy(darr)                                                     \
  do {                                                                         \
    DarrHdr *hdr = DARR_HDR(darr);                                             \
                                                                               \
    if (hdr->destructor) {                                                     \
      for (size_t i = 0; i < hdr->size; ++i) {                                 \
        hdr->destructor(&(darr)[i]);                                           \
      }                                                                        \
    }                                                                          \
                                                                               \
    free(hdr);                                                                 \
    (darr) = NULL;                                                             \
  } while (0);

#define darr_clear(darr)                                                       \
  do {                                                                         \
    DarrHdr *hdr = DARR_HDR(darr);                                             \
                                                                               \
    for (size_t i = 0; i < hdr->size; ++i) {                                   \
      if (hdr->destructor) {                                                   \
        hdr->destructor(&(darr)[i]);                                           \
      }                                                                        \
    }                                                                          \
                                                                               \
    hdr->size = 0;                                                             \
  } while (0);

#endif // !C_UTILS_DYNAMIC_ARRAYS
