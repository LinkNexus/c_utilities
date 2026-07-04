#include "../arena.h"
#include "../test_utils.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int x;
  int y;
} Vec2;

typedef struct {
  char name[32];
  int age;
} Person;

void test_arena_creation() {
  TEST_SUITE("Arena creation");

  Arena arena = arena_create(1024);

  TEST_ASSERT(arena.head != NULL, "Arena head block must not be NULL");

  TEST_ASSERT(arena.current != NULL, "Arena current block must not be NULL");

  TEST_ASSERT(arena.head == arena.current,
              "Head and current must initially be identical");

  TEST_ASSERT(arena.default_block_size == 1024,
              "Default block size must be 1024, got %zu",
              arena.default_block_size);

  TEST_ASSERT(arena.head->cap == 1024,
              "Initial block capacity must be 1024, got %zu", arena.head->cap);

  TEST_ASSERT(arena.head->offset == 0, "Initial offset must be 0, got %zu",
              arena.head->offset);

  arena_destroy(&arena);
}

void test_basic_allocation() {
  TEST_SUITE("Basic allocation");

  Arena arena = arena_create(1024);

  int *value = arena_alloc(&arena, sizeof(int));

  TEST_ASSERT(value != NULL, "arena_alloc must return non-NULL");

  *value = 42;

  TEST_ASSERT(*value == 42, "Allocated integer must store correct value");

  TEST_ASSERT(arena.current->offset >= sizeof(int),
              "Offset must advance after allocation");

  arena_destroy(&arena);
}

void test_multiple_allocations() {
  TEST_SUITE("Multiple allocations");

  Arena arena = arena_create(1024);

  int *a = arena_alloc(&arena, sizeof(int));
  int *b = arena_alloc(&arena, sizeof(int));
  int *c = arena_alloc(&arena, sizeof(int));

  *a = 10;
  *b = 20;
  *c = 30;

  TEST_ASSERT(*a == 10, "Value a must equal 10");
  TEST_ASSERT(*b == 20, "Value b must equal 20");
  TEST_ASSERT(*c == 30, "Value c must equal 30");

  TEST_ASSERT(a != b, "Separate allocations must return distinct pointers");

  TEST_ASSERT(b != c, "Separate allocations must return distinct pointers");

  arena_destroy(&arena);
}

void test_alignment() {
  TEST_SUITE("Alignment");

  Arena arena = arena_create(1024);

  char *c = arena_alloc(&arena, sizeof(char));
  double *d = arena_alloc(&arena, sizeof(double));

  TEST_ASSERT(c != NULL, "char allocation must succeed");

  TEST_ASSERT(d != NULL, "double allocation must succeed");

  TEST_ASSERT(((uintptr_t)d % ARENA_ALIGNMENT) == 0,
              "double allocation must be aligned to %zu bytes",
              (size_t)ARENA_ALIGNMENT);

  arena_destroy(&arena);
}

void test_block_growth() {
  TEST_SUITE("Block growth");

  Arena arena = arena_create(32);

  void *a = arena_alloc(&arena, 24);
  void *b = arena_alloc(&arena, 24);

  TEST_ASSERT(a != NULL, "First allocation must succeed");

  TEST_ASSERT(b != NULL, "Second allocation must succeed");

  TEST_ASSERT(arena.head != arena.current,
              "Arena must allocate a second block");

  TEST_ASSERT(arena.head->next == arena.current,
              "Head next must point to current block");

  arena_destroy(&arena);
}

void test_large_allocation() {
  TEST_SUITE("Large allocation");

  Arena arena = arena_create(64);

  void *ptr = arena_alloc(&arena, 4096);

  TEST_ASSERT(ptr != NULL, "Large allocation must succeed");

  TEST_ASSERT(arena.current->cap >= 4096,
              "New block capacity must fit large allocation");

  arena_destroy(&arena);
}

void test_calloc() {
  TEST_SUITE("Calloc");

  Arena arena = arena_create(1024);

  int *values = arena_calloc(&arena, 16, sizeof(int));

  TEST_ASSERT(values != NULL, "arena_calloc must return non-NULL");

  int all_zero = 1;

  for (size_t i = 0; i < 16; i++) {
    if (values[i] != 0) {
      all_zero = 0;
      break;
    }
  }

  TEST_ASSERT(all_zero, "arena_calloc memory must be zero initialized");

  arena_destroy(&arena);
}

void test_reset() {
  TEST_SUITE("Reset");

  Arena arena = arena_create(1024);

  arena_alloc(&arena, 128);
  arena_alloc(&arena, 256);

  TEST_ASSERT(arena.current->offset > 0, "Offset must advance before reset");

  arena_reset(&arena);

  TEST_ASSERT(arena.head->offset == 0, "Head offset must be reset to 0");

  TEST_ASSERT(arena.current == arena.head, "Current block must reset to head");

  arena_destroy(&arena);
}

void test_reuse_after_reset() {
  TEST_SUITE("Reuse after reset");

  Arena arena = arena_create(1024);

  int *a = arena_alloc(&arena, sizeof(int));

  arena_reset(&arena);

  int *b = arena_alloc(&arena, sizeof(int));

  TEST_ASSERT(a == b, "Arena must reuse memory after reset");

  arena_destroy(&arena);
}

void test_trim() {
  TEST_SUITE("Trim");

  Arena arena = arena_create(64);

  arena_alloc(&arena, 48);
  arena_alloc(&arena, 48);
  arena_alloc(&arena, 48);

  TEST_ASSERT(arena.head != arena.current,
              "Arena must contain multiple blocks before trim");

  arena_trim(&arena);

  TEST_ASSERT(arena.current == arena.head,
              "Current must point to head after trim");

  TEST_ASSERT(arena.head->next == NULL,
              "All extra blocks must be freed after trim");

  arena_destroy(&arena);
}

void test_struct_allocation() {
  TEST_SUITE("Struct allocation");

  Arena arena = arena_create(1024);

  Person *person = arena_alloc(&arena, sizeof(Person));

  strcpy(person->name, "Levy");
  person->age = 22;

  TEST_ASSERT(strcmp(person->name, "Levy") == 0,
              "Struct string field must match");

  TEST_ASSERT(person->age == 22, "Struct integer field must match");

  arena_destroy(&arena);
}

void test_string_utils() {
  TEST_SUITE("String utils");

  Arena arena = arena_create(256);

  char *s = arena_strdup(&arena, "hello");
  TEST_ASSERT(strcmp(s, "hello") == 0, "arena_strdup must copy string");

  char *t = arena_strndup(&arena, "hello world", 5);
  TEST_ASSERT(strcmp(t, "hello") == 0, "arena_strndup must copy prefix");

  int src[4] = {1, 2, 3, 4};
  int *dup = arena_memdup(&arena, src, sizeof src);
  TEST_ASSERT(dup != NULL, "arena_memdup must return non-NULL");
  TEST_ASSERT(memcmp(dup, src, sizeof src) == 0,
              "arena_memdup must copy bytes");

  arena_destroy(&arena);
}

int main(void) {
  test_arena_creation();
  test_basic_allocation();
  test_multiple_allocations();
  test_alignment();
  test_block_growth();
  test_large_allocation();
  test_calloc();
  test_reset();
  test_reuse_after_reset();
  test_trim();
  test_struct_allocation();
  test_string_utils();

  TEST_SUMMARY();

  return _test_fail_count > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
