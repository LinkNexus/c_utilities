#ifndef C_UTILS_ARENA_IMPLEMENTATION
#define C_UTILS_ARENA_IMPLEMENTATION

#include "utils.h"
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct ArenaBlock {
  struct ArenaBlock* next;
  size_t cap;
  size_t offset;
  unsigned char buf[];
} ArenaBlock;

typedef struct {
  ArenaBlock* head;
  ArenaBlock* current;
  size_t default_block_size;
} Arena;

#define ARENA_ALIGNMENT alignof(max_align_t)

static inline size_t arena_align_forward(size_t offset, size_t alignment) {
  return (offset + alignment - 1) & ~(alignment - 1);
}

static inline ArenaBlock* arena_allocate_block(size_t block_size) {
  ArenaBlock* block = (ArenaBlock*)xmalloc(block_size + sizeof *block);

  block->next = NULL;
  block->cap = block_size;
  block->offset = 0;

  return block;
}

static inline Arena arena_create(size_t block_size) {
  ArenaBlock* block = arena_allocate_block(block_size);
  return (Arena){.head = block, .current = block, .default_block_size = block_size};
}

static inline void arena_destroy(Arena* arena) {
  ArenaBlock* current = arena->head;

  while (current) {
    ArenaBlock* block = current->next;
    free(current);
    current = block;
  }

  arena->head = NULL;
  arena->current = NULL;
}

static inline void* arena_alloc(Arena* arena, size_t size) {
  size_t aligned = arena_align_forward(arena->current->offset, ARENA_ALIGNMENT);

  if (arena->current->cap - aligned < size) {
    ArenaBlock* block =
        arena_allocate_block(MAX(arena->default_block_size, size + ARENA_ALIGNMENT));
    arena->current->next = block;
    arena->current = block;
    aligned = 0;
  }

  void* ptr = arena->current->buf + aligned;
  arena->current->offset = aligned + size;
  return ptr;
}

static inline void* arena_memdup(Arena* arena, const void* src, size_t size) {
  if (size == 0)
    return NULL;
  void* dst = arena_alloc(arena, size);
  memcpy(dst, src, size);
  return dst;
}

static inline char* arena_strndup(Arena* arena, const char* src, size_t len) {
  char* dst = arena_alloc(arena, len + 1);
  memcpy(dst, src, len);
  dst[len] = '\0';
  return dst;
}

static inline char* arena_strdup(Arena* arena, const char* src) {
  return arena_strndup(arena, src, strlen(src));
}

static inline void* arena_calloc(Arena* arena, size_t count, size_t element_size) {
  if (count != 0 && element_size > SIZE_MAX / count) {
    print_fn_err_msg("arena_calloc: Couldn't allocate memory; Overflow!");
  }

  size_t total_size = count * element_size;
  void* ptr = arena_alloc(arena, total_size);

  memset(ptr, 0, total_size);
  return ptr;
}

static inline void arena_reset(Arena* arena) {
  ArenaBlock* current = arena->head;

  while (current) {
    current->offset = 0;
    current = current->next;
  }

  arena->current = arena->head;
}

static inline void arena_trim(Arena* arena) {
  ArenaBlock* current = arena->head->next;
  arena->head->next = NULL;

  while (current) {
    ArenaBlock* block = current->next;
    free(current);
    current = block;
  }

  arena->current = arena->head;
}

#endif // !C_UTILS_ARENA_IMPLEMENTATION
