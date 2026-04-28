#ifndef C_UTILS_HASHMAPS_IMPLEMENTATION
#define C_UTILS_HASHMAPS_IMPLEMENTATION

#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define HASHMAP_MIN_CAP 4
#define HASHMAP_LOAD_FACTOR 0.75

typedef enum {
  HASHMAPENTRY_EMPTY,
  HASHMAPENTRY_OCCUPIED,
  HASHMAPENTRY_DELETED
} HashMapEntryState;

typedef struct {
  size_t cap;
  size_t size;
  void (*destructor)(void *value);
  size_t deleted_count;
  size_t entry_size;
  size_t value_offset;
} HashMapHdr;

#define HASHENTRY_FIELDS(type)                                                 \
  const char *key;                                                             \
  HashMapEntryState state;                                                     \
  type value;

#define HASHENTRY_OF(type) {HASHENTRY_FIELDS(type)}

#define HASHMAP_HDR(entries) ((HashMapHdr *)(entries) - 1)

#define HASHMAP_GET_ENTRY(entries, idx)                                        \
  ((void *)((char *)(entries) + (idx) * HASHMAP_HDR(entries)->entry_size))

#define HASHMAP_ENTRY_KEY(entries, idx)                                        \
  (*(const char **)((char *)HASHMAP_GET_ENTRY(entries, idx)))

#define HASHMAP_ENTRY_STATE(entries, idx)                                      \
  (*(HashMapEntryState *)((char *)HASHMAP_GET_ENTRY(entries, idx) +            \
                          sizeof(const char *)))

#define HASHMAP_ENTRY_VALUE(entries, idx)                                      \
  ((void *)((char *)HASHMAP_GET_ENTRY(entries, idx) +                          \
            HASHMAP_HDR(entries)->value_offset))

static inline unsigned long hashmap_hash_key(const char *str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;

  return hash;
}

#define hashmap_init_entries(entries)                                          \
  do {                                                                         \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
                                                                               \
    for (size_t i = 0; i < hdr->cap; ++i) {                                    \
      HASHMAP_ENTRY_KEY(entries, i) = NULL;                                    \
      HASHMAP_ENTRY_STATE(entries, i) = HASHMAPENTRY_EMPTY;                    \
    }                                                                          \
  } while (0);

#define hashmap_init(entries)                                                  \
  do {                                                                         \
    if ((entries))                                                             \
      break;                                                                   \
                                                                               \
    size_t entry_size = sizeof *(entries);                                     \
                                                                               \
    HashMapHdr *hdr = xmalloc(HASHMAP_MIN_CAP * entry_size + sizeof *hdr);     \
    hdr->cap = HASHMAP_MIN_CAP;                                                \
    (entries) = (void *)(hdr + 1);                                             \
    hdr->size = 0;                                                             \
    hdr->deleted_count = 0;                                                    \
    hdr->entry_size = entry_size;                                              \
    hdr->value_offset = (char *)&((entries)->value) - (char *)(entries);       \
                                                                               \
    hashmap_init_entries(entries);                                             \
  } while (0);

#define hashmap_set_destructor(entries, fn)                                    \
  do {                                                                         \
    hashmap_init(entries);                                                     \
                                                                               \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
    hdr->destructor = (void (*)(void *))fn;                                    \
  } while (0);

#define hashmap_resize(entries, new_cap)                                       \
  do {                                                                         \
    hashmap_init(entries);                                                     \
                                                                               \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
    new_cap = MAX(new_cap, HASHMAP_MIN_CAP);                                   \
                                                                               \
    if (new_cap == hdr->cap)                                                   \
      break;                                                                   \
                                                                               \
    HashMapHdr *new_hdr =                                                      \
        xmalloc(new_cap * hdr->entry_size + sizeof *new_hdr);                  \
    char *new_entries = (char *)(new_hdr + 1);                                 \
                                                                               \
    new_hdr->cap = new_cap;                                                    \
    new_hdr->size = hdr->size;                                                 \
    new_hdr->destructor = hdr->destructor;                                     \
    new_hdr->deleted_count = 0;                                                \
    new_hdr->entry_size = hdr->entry_size;                                     \
    new_hdr->value_offset = hdr->value_offset;                                 \
                                                                               \
    hashmap_init_entries(new_entries);                                         \
                                                                               \
    for (size_t i = 0; i < hdr->cap; ++i) {                                    \
      if (HASHMAP_ENTRY_STATE(entries, i) == HASHMAPENTRY_OCCUPIED) {          \
        size_t idx =                                                           \
            hashmap_hash_key(HASHMAP_ENTRY_KEY(entries, i)) % new_cap;         \
                                                                               \
        while (HASHMAP_ENTRY_STATE(new_entries, idx) ==                        \
               HASHMAPENTRY_OCCUPIED) {                                        \
          idx = (idx + 1) % new_cap;                                           \
        }                                                                      \
                                                                               \
        memcpy(HASHMAP_GET_ENTRY(new_entries, idx),                            \
               HASHMAP_GET_ENTRY(entries, i), hdr->entry_size);                \
      }                                                                        \
    }                                                                          \
                                                                               \
    free(hdr);                                                                 \
    (entries) = (void *)(new_entries);                                         \
  } while (0);

#define hashmap_ensure_cap(entries)                                            \
  do {                                                                         \
    hashmap_init(entries);                                                     \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
                                                                               \
    if (((float)(hdr->size + hdr->deleted_count) / hdr->cap) <                 \
        HASHMAP_LOAD_FACTOR)                                                   \
      break;                                                                   \
                                                                               \
    size_t new_cap = hdr->cap * 2;                                             \
    hashmap_resize(entries, new_cap);                                          \
  } while (0);

#define hashmap_set(entries, k, v)                                             \
  do {                                                                         \
    hashmap_ensure_cap(entries);                                               \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
                                                                               \
    size_t idx = hashmap_hash_key(k) % hdr->cap;                               \
    size_t checked = 0;                                                        \
    size_t first_deleted_idx = (size_t)-1;                                     \
                                                                               \
    while (checked < hdr->cap) {                                               \
      HashMapEntryState state = HASHMAP_ENTRY_STATE(entries, idx);             \
                                                                               \
      if (state == HASHMAPENTRY_EMPTY) {                                       \
        if (first_deleted_idx != (size_t)-1) {                                 \
          idx = first_deleted_idx;                                             \
          hdr->deleted_count--;                                                \
        }                                                                      \
                                                                               \
        HASHMAP_ENTRY_KEY(entries, idx) = strdup(k);                           \
        HASHMAP_ENTRY_STATE(entries, idx) = HASHMAPENTRY_OCCUPIED;             \
        memcpy(HASHMAP_ENTRY_VALUE(entries, idx), v, sizeof(*(v)));            \
        hdr->size++;                                                           \
        break;                                                                 \
      }                                                                        \
                                                                               \
      else if (state == HASHMAPENTRY_DELETED) {                                \
        if (first_deleted_idx == (size_t)-1)                                   \
          first_deleted_idx = idx;                                             \
      }                                                                        \
                                                                               \
      else if (strcmp(HASHMAP_ENTRY_KEY(entries, idx), k) == 0) {              \
        if (hdr->destructor)                                                   \
          hdr->destructor(HASHMAP_ENTRY_VALUE(entries, idx));                  \
        memcpy(HASHMAP_ENTRY_VALUE(entries, idx), v, sizeof(*(v)));            \
        break;                                                                 \
      }                                                                        \
                                                                               \
      idx = (idx + 1) % hdr->cap;                                              \
      checked++;                                                               \
    }                                                                          \
  } while (0)

static inline size_t hashmap_get_idx(void *entries, const char *key) {
  HashMapHdr *hdr = HASHMAP_HDR(entries);
  size_t idx = hashmap_hash_key(key) % hdr->cap;
  size_t checked = 0;

  while (checked < hdr->cap) {
    HashMapEntryState state = HASHMAP_ENTRY_STATE(entries, idx);

    if (state == HASHMAPENTRY_EMPTY)
      break;

    if (state == HASHMAPENTRY_OCCUPIED &&
        strcmp(HASHMAP_ENTRY_KEY(entries, idx), key) == 0)
      return idx;

    idx = (idx + 1) % hdr->cap;
    checked++;
  }

  return SIZE_MAX;
}

static inline void *hashmap_get_ref_impl(void *entries, const char *key) {
  size_t idx = hashmap_get_idx(entries, key);
  return idx == SIZE_MAX ? NULL : HASHMAP_ENTRY_VALUE(entries, idx);
}

static inline bool hashmap_get_impl(void *entries, const char *key, void *out,
                                    size_t value_size) {
  size_t idx = hashmap_get_idx(entries, key);

  if (idx == SIZE_MAX)
    return false;

  memcpy(out, HASHMAP_ENTRY_VALUE(entries, idx), value_size);
  return true;
}

#define hashmap_get_ref(entries, k, out)                                       \
  (((out) = hashmap_get_ref_impl(entries, k)) != NULL)

#define hashmap_get(entries, k, out)                                           \
  (hashmap_get_impl(entries, k, &out, sizeof(out)))

#define hashmap_delete(entries, k)                                             \
  do {                                                                         \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
    size_t idx = hashmap_get_idx(entries, k);                                  \
                                                                               \
    if (idx == SIZE_MAX)                                                       \
      break;                                                                   \
                                                                               \
    if (hdr->destructor)                                                       \
      hdr->destructor(HASHMAP_ENTRY_VALUE(entries, idx));                      \
                                                                               \
    free(HASHMAP_ENTRY_KEY(entries, idx));                                     \
                                                                               \
    (HASHMAP_ENTRY_KEY(entries, idx)) = NULL;                                  \
    (HASHMAP_ENTRY_STATE(entries, idx)) = HASHMAPENTRY_DELETED;                \
                                                                               \
    hdr->size--;                                                               \
    hdr->deleted_count++;                                                      \
  } while (0);

#define hashmap_iterate(entries, fn)                                           \
  do {                                                                         \
    hashmap_init(entries);                                                     \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
                                                                               \
    for (size_t i = 0; i < hdr->cap; i++) {                                    \
      if (HASHMAP_ENTRY_STATE(entries, i) == HASHMAPENTRY_OCCUPIED)            \
        fn(HASHMAP_GET_ENTRY(entries, i), i);                                  \
    }                                                                          \
  } while (0);

#define hashmap_iterate_keys(entries, fn)                                      \
  do {                                                                         \
    hashmap_init(entries);                                                     \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
                                                                               \
    for (size_t i = 0; i < hdr->cap; i++) {                                    \
      if (HASHMAP_ENTRY_STATE(entries, i) == HASHMAPENTRY_OCCUPIED)            \
        fn(HASHMAP_ENTRY_KEY(entries, i), i);                                  \
    }                                                                          \
  } while (0);

#define hashmap_iterate_values(entries, fn)                                    \
  do {                                                                         \
    hashmap_init(entries);                                                     \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
                                                                               \
    for (size_t i = 0; i < hdr->cap; i++) {                                    \
      if (HASHMAP_ENTRY_STATE(entries, i) == HASHMAPENTRY_OCCUPIED)            \
        fn(HASHMAP_ENTRY_VALUE(entries, i), i);                                \
    }                                                                          \
  } while (0);

#endif // !C_UTILS_HASHMAPS_IMPLEMENTATION
