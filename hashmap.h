#ifndef C_UTILS_HASHMAPS
#define C_UTILS_HASHMAPS

#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define HASHMAP_MIN_CAP 4
#define HASHMAP_LOAD_FACTOR 0.75

typedef enum {
  HASHMAP_ENTRY_EMPTY,
  HASHMAP_ENTRY_OCCUPIED,
  HASHMAP_ENTRY_DELETED,
} HASHMAP_ENTRY_STATE;

#define HASHMAP_ENTRIES_FIELDS(type)                                           \
  const char *key;                                                             \
  HASHMAP_ENTRY_STATE state;                                                   \
  type value;

#define HASHMAP_ENTRY_OF(type) {HASHMAP_ENTRIES_FIELDS(type)}

typedef struct {
  size_t cap;
  size_t size;
  size_t deleted_count;
  size_t entry_size;
  size_t value_offset;
  void (*destructor)(void *value);
} HashMapHdr;

static unsigned long hashmap_hash_key(const char *str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;

  return hash;
}

#define HASHMAP_HDR(entries) ((HashMapHdr *)(entries) - 1)

#define hashmap_init_entries(entries)                                          \
  do {                                                                         \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
                                                                               \
    for (size_t i = 0; i < hdr->cap; i++) {                                    \
      typeof(entries) entry = &(entries)[i];                                   \
      entry->key = NULL;                                                       \
      entry->state = HASHMAP_ENTRY_EMPTY;                                      \
    }                                                                          \
  } while (0);

#define hashmap_init(entries)                                                  \
  do {                                                                         \
    if (entries)                                                               \
      break;                                                                   \
    HashMapHdr *hdr =                                                          \
        xmalloc(HASHMAP_MIN_CAP * sizeof *(entries) + sizeof *hdr);            \
    entries = (void *)(hdr + 1);                                               \
    hdr->cap = HASHMAP_MIN_CAP;                                                \
    hdr->size = 0;                                                             \
    hdr->deleted_count = 0;                                                    \
    hdr->destructor = NULL;                                                    \
    hdr->entry_size = sizeof *(entries);                                       \
    hdr->value_offset = offsetof(typeof(*(entries)), value);                   \
                                                                               \
    hashmap_init_entries(entries);                                             \
  } while (0);

#define hashmap_set_destructor(entries, fn)                                    \
  ((HASHMAP_HDR(entries))->destructor = (void (*)(void *))(fn))

#define hashmap_resize(entries, new_cap)                                       \
  do {                                                                         \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
    size_t real_new_cap = MAX(new_cap, HASHMAP_MIN_CAP);                       \
                                                                               \
    if (real_new_cap == hdr->cap)                                              \
      break;                                                                   \
                                                                               \
    HashMapHdr *new_hdr =                                                      \
        xmalloc(real_new_cap * sizeof *(entries) + sizeof *new_hdr);           \
    new_hdr->cap = real_new_cap;                                               \
    new_hdr->size = hdr->size;                                                 \
    new_hdr->destructor = hdr->destructor;                                     \
    new_hdr->deleted_count = 0;                                                \
    new_hdr->entry_size = hdr->entry_size;                                     \
    new_hdr->value_offset = hdr->value_offset;                                 \
                                                                               \
    typeof(entries) new_entries = (typeof(entries))(new_hdr + 1);              \
                                                                               \
    hashmap_init_entries(new_entries);                                         \
                                                                               \
    for (size_t i = 0; i < hdr->cap; i++) {                                    \
      typeof(*entries) entry = (entries)[i];                                   \
                                                                               \
      if (entry.state == HASHMAP_ENTRY_OCCUPIED) {                             \
        size_t idx = hashmap_hash_key(entry.key) % real_new_cap;               \
                                                                               \
        while (new_entries[idx].state == HASHMAP_ENTRY_OCCUPIED)               \
          idx = (idx + 1) % real_new_cap;                                      \
                                                                               \
        new_entries[idx] = entry;                                              \
      }                                                                        \
    }                                                                          \
                                                                               \
    free(hdr);                                                                 \
    (entries) = (void *)new_entries;                                           \
  } while (0);

#define hashmap_ensure_cap(entries)                                            \
  do {                                                                         \
    hashmap_init(entries);                                                     \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
                                                                               \
    if ((((float)(hdr->size + hdr->deleted_count)) / hdr->cap) <               \
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
      typeof(entries) entry = &(entries)[idx];                                 \
                                                                               \
      if (entry->state == HASHMAP_ENTRY_EMPTY) {                               \
        if (first_deleted_idx != (size_t)-1) {                                 \
          idx = first_deleted_idx;                                             \
          hdr->deleted_count--;                                                \
        }                                                                      \
                                                                               \
        entry = &(entries)[idx];                                               \
        entry->key = strdup(k);                                                \
        entry->state = HASHMAP_ENTRY_OCCUPIED;                                 \
        entry->value = v;                                                      \
        hdr->size++;                                                           \
        break;                                                                 \
      }                                                                        \
                                                                               \
      else if (entry->state == HASHMAP_ENTRY_DELETED) {                        \
        if (first_deleted_idx == (size_t)-1)                                   \
          first_deleted_idx = idx;                                             \
      }                                                                        \
                                                                               \
      else if (strcmp(entry->key, k) == 0) {                                   \
        if (hdr->destructor)                                                   \
          hdr->destructor(&entry->value);                                      \
        entry->value = v;                                                      \
        break;                                                                 \
      }                                                                        \
                                                                               \
      idx = (idx + 1) % hdr->cap;                                              \
      checked++;                                                               \
    }                                                                          \
  } while (0);

static inline size_t hashmap_get_idx(void *entries, const char *key) {
  HashMapHdr *hdr = HASHMAP_HDR(entries);

  size_t idx = hashmap_hash_key(key) % hdr->cap;
  size_t checked = 0;

  while (checked < hdr->cap) {
    char *entry = (char *)entries + idx * hdr->entry_size;
    const char *entry_key = *(const char **)entry;
    HASHMAP_ENTRY_STATE state =
        *(HASHMAP_ENTRY_STATE *)(entry + sizeof(const char *));

    if (state == HASHMAP_ENTRY_EMPTY)
      break;

    if (state == HASHMAP_ENTRY_OCCUPIED && strcmp(entry_key, key) == 0)
      return idx;

    idx = (idx + 1) % hdr->cap;
    checked++;
  }

  return SIZE_MAX;
}

static inline void *hashmap_get_ref_impl(void *entries, const char *key) {
  HashMapHdr *hdr = HASHMAP_HDR(entries);
  size_t idx = hashmap_get_idx(entries, key);

  if (idx == SIZE_MAX)
    return NULL;

  return (void *)((char *)entries + idx * hdr->entry_size + hdr->value_offset);
}

static inline bool hashmap_get_impl(void *entries, const char *key, void *out) {
  HashMapHdr *hdr = HASHMAP_HDR(entries);
  void *ref = hashmap_get_ref_impl(entries, key);
  if (!ref)
    return false;

  memcpy(out, ref, hdr->entry_size - hdr->value_offset);
  return true;
}

#define hashmap_get_ref(entries, k, out)                                       \
  ((*(out) = hashmap_get_ref_impl(entries, k)) != NULL)

#define hashmap_get(entries, k, out) (hashmap_get_impl(entries, k, out))

#define hashmap_delete(entries, k)                                             \
  do {                                                                         \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
    size_t idx = hashmap_get_idx(entries, k);                                  \
                                                                               \
    if (idx == SIZE_MAX)                                                       \
      break;                                                                   \
                                                                               \
    typeof(entries) entry = &(entries)[idx];                                   \
                                                                               \
    if (hdr->destructor)                                                       \
      hdr->destructor(&entry->value);                                          \
                                                                               \
    free((char *)entry->key);                                                  \
                                                                               \
    entry->key = NULL;                                                         \
    entry->state = HASHMAP_ENTRY_DELETED;                                      \
    hdr->size--;                                                               \
    hdr->deleted_count++;                                                      \
  } while (0);

#define hashmap_iterate(entries, fn)                                           \
  do {                                                                         \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
                                                                               \
    for (size_t i = 0; i < hdr->cap; ++i) {                                    \
      typeof(entries) entry = &(entries)[i];                                   \
      if (entry->state == HASHMAP_ENTRY_OCCUPIED)                              \
        fn(entry, i);                                                          \
    }                                                                          \
  } while (0);

#define hashmap_iterate_keys(entries, fn)                                      \
  do {                                                                         \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
                                                                               \
    for (size_t i = 0; i < hdr->cap; ++i) {                                    \
      typeof(entries) entry = &(entries)[i];                                   \
      if (entry->state == HASHMAP_ENTRY_OCCUPIED)                              \
        fn(entry->key, i);                                                     \
    }                                                                          \
  } while (0);

#define hashmap_iterate_values(entries, fn)                                    \
  do {                                                                         \
    HashMapHdr *hdr = HASHMAP_HDR(entries);                                    \
                                                                               \
    for (size_t i = 0; i < hdr->cap; ++i) {                                    \
      typeof(entries) entry = &(entries)[i];                                   \
      if (entry->state == HASHMAP_ENTRY_OCCUPIED)                              \
        fn(&(entry->value), i);                                                \
    }                                                                          \
  } while (0);

#endif // !C_UTILS_HASHMAPS
