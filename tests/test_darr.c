#include "../darr.h"
#include "../test_utils.h"
#include "../arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_init() {
  TEST_SUITE("Init");

  int* arr = NULL;
  darr_init(arr);

  TEST_ASSERT(arr != NULL, "Array must not be NULL after init");
  TEST_ASSERT(darr_len(arr) == 0, "Length must be 0 after init, got %zu", darr_len(arr));
  TEST_ASSERT(darr_cap(arr) == MIN_DYNAMIC_ARRAY_SIZE, "Cap must be %d after init, got %zu",
              MIN_DYNAMIC_ARRAY_SIZE, darr_cap(arr));

  // calling init again must be a no-op
  DarrHdr* hdr_before = DARR_HDR(arr);
  darr_init(arr);
  TEST_ASSERT(DARR_HDR(arr) == hdr_before, "Second init must not reallocate");
}

void test_arena_init_append() {
  TEST_SUITE("Arena init/append");

  Arena arena = arena_create(256);
  int* arr = NULL;

  arena_darr_init(&arena, arr);
  TEST_ASSERT(arr != NULL, "Array must not be NULL after arena init");
  TEST_ASSERT(darr_len(arr) == 0, "Length must be 0 after arena init, got %zu", darr_len(arr));

  for (int i = 0; i < 16; i++)
    arena_darr_append(&arena, arr, i);

  TEST_ASSERT(darr_len(arr) == 16, "Length must be 16, got %zu", darr_len(arr));
  for (int i = 0; i < 16; i++)
    TEST_ASSERT(arr[i] == i, "arr[%d] must be %d, got %d", i, i, arr[i]);

  arena_destroy(&arena);
}

void test_arena_reserve_safety() {
  TEST_SUITE("Arena reserve safety");

  Arena arena = arena_create(256);
  int* arr = NULL;

  arena_darr_init(&arena, arr);
  for (int i = 0; i < 4; i++)
    arena_darr_append(&arena, arr, i);

  // Allocate something else after the darr, so it is not the last allocation.
  char* guard = arena_alloc(&arena, 64);
  memset(guard, 0xAB, 64);

  // This must NOT clobber 'guard'. A buggy in-place reserve will.
  arena_darr_reserve(&arena, arr, 128);
  for (int i = 0; i < 4; i++)
    TEST_ASSERT(arr[i] == i, "arr[%d] must remain %d after reserve", i, i);

  for (int i = 0; i < 64; i++)
    TEST_ASSERT(((unsigned char*)guard)[i] == 0xAB,
                "guard byte %d must remain intact after reserve", i);

  arena_destroy(&arena);
}

void test_append() {
  TEST_SUITE("Append");

  int* arr = NULL;
  darr_append(arr, 10);
  darr_append(arr, 20);
  darr_append(arr, 30);

  TEST_ASSERT(darr_len(arr) == 3, "Length must be 3, got %zu", darr_len(arr));
  TEST_ASSERT(arr[0] == 10, "arr[0] must be 10, got %d", arr[0]);
  TEST_ASSERT(arr[1] == 20, "arr[1] must be 20, got %d", arr[1]);
  TEST_ASSERT(arr[2] == 30, "arr[2] must be 30, got %d", arr[2]);
}

void test_resize() {
  TEST_SUITE("Resize / grow");

  int* arr = NULL;
  for (int i = 0; i < 16; i++)
    darr_append(arr, i);

  TEST_ASSERT(darr_len(arr) == 16, "Length must be 16 after 16 appends, got %zu", darr_len(arr));
  TEST_ASSERT(darr_cap(arr) >= 16, "Cap must be >= 16, got %zu", darr_cap(arr));

  for (int i = 0; i < 16; i++)
    TEST_ASSERT(arr[i] == i, "arr[%d] must be %d, got %d", i, i, arr[i]);
}

void test_pop() {
  TEST_SUITE("Pop");

  int* arr = NULL;
  darr_append(arr, 1);
  darr_append(arr, 2);
  darr_append(arr, 3);

  darr_pop(arr);
  TEST_ASSERT(darr_len(arr) == 2, "Length must be 2 after pop, got %zu", darr_len(arr));
  TEST_ASSERT(arr[0] == 1, "arr[0] must still be 1, got %d", arr[0]);
  TEST_ASSERT(arr[1] == 2, "arr[1] must still be 2, got %d", arr[1]);

  darr_pop(arr);
  darr_pop(arr);
  TEST_ASSERT(darr_len(arr) == 0, "Length must be 0 after popping all, got %zu", darr_len(arr));

  // pop on empty must not crash
  darr_pop(arr);
  TEST_ASSERT(darr_len(arr) == 0, "Pop on empty must be a no-op, got %zu", darr_len(arr));
}

void test_set() {
  TEST_SUITE("Set");

  int* arr = NULL;
  darr_append(arr, 1);
  darr_append(arr, 2);
  darr_append(arr, 3);

  darr_set(arr, 1, 42);
  TEST_ASSERT(arr[1] == 42, "arr[1] must be 42 after set, got %d", arr[1]);
  TEST_ASSERT(darr_len(arr) == 3, "Length must remain 3, got %zu", darr_len(arr));

  // set at size — acts like append
  darr_set(arr, 3, 99);
  TEST_ASSERT(darr_len(arr) == 4, "Length must be 4 after set at end, got %zu", darr_len(arr));
  TEST_ASSERT(arr[3] == 99, "arr[3] must be 99, got %d", arr[3]);
}

void test_insert_at() {
  TEST_SUITE("Insert at");

  int* arr = NULL;
  darr_append(arr, 1);
  darr_append(arr, 2);
  darr_append(arr, 3);

  darr_insert_at(arr, 1, 42);

  TEST_ASSERT(darr_len(arr) == 4, "Length must be 4 after insert, got %zu", darr_len(arr));
  TEST_ASSERT(arr[0] == 1, "arr[0] must be 1, got %d", arr[0]);
  TEST_ASSERT(arr[1] == 42, "arr[1] must be 42, got %d", arr[1]);
  TEST_ASSERT(arr[2] == 2, "arr[2] must be 2, got %d", arr[2]);
  TEST_ASSERT(arr[3] == 3, "arr[3] must be 3, got %d", arr[3]);

  // insert at 0
  darr_insert_at(arr, 0, 99);
  TEST_ASSERT(arr[0] == 99, "arr[0] must be 99 after insert at 0, got %d", arr[0]);
  TEST_ASSERT(darr_len(arr) == 5, "Length must be 5, got %zu", darr_len(arr));

  // insert at end
  darr_insert_at(arr, darr_len(arr), 77);
  TEST_ASSERT(arr[darr_len(arr) - 1] == 77, "Last element must be 77 after insert at end, got %d",
              arr[darr_len(arr) - 1]);
}

void test_remove_at() {
  TEST_SUITE("Remove at");

  int* arr = NULL;
  darr_append(arr, 10);
  darr_append(arr, 20);
  darr_append(arr, 30);
  darr_append(arr, 40);

  darr_remove_at(arr, 1);

  TEST_ASSERT(darr_len(arr) == 3, "Length must be 3 after remove, got %zu", darr_len(arr));
  TEST_ASSERT(arr[0] == 10, "arr[0] must be 10, got %d", arr[0]);
  TEST_ASSERT(arr[1] == 30, "arr[1] must be 30, got %d", arr[1]);
  TEST_ASSERT(arr[2] == 40, "arr[2] must be 40, got %d", arr[2]);

  // remove first
  darr_remove_at(arr, 0);
  TEST_ASSERT(arr[0] == 30, "arr[0] must be 30 after removing first, got %d", arr[0]);

  printf("len=%zu, removing idx=%zu\n", darr_len(arr), darr_len(arr) - 1);
  // darr_remove_at(arr, darr_len(arr) - 1);

  // remove last
  darr_remove_at(arr, darr_len(arr) - 1);
  TEST_ASSERT(darr_len(arr) == 1, "Length must be 1, got %zu", darr_len(arr));
}

void test_append_range() {
  TEST_SUITE("Append range");

  int* arr = NULL;
  darr_append(arr, 1);

  int extra[] = {2, 3, 4, 5};
  darr_append_range(arr, extra, 4);

  TEST_ASSERT(darr_len(arr) == 5, "Length must be 5, got %zu", darr_len(arr));
  for (int i = 0; i < 5; i++)
    TEST_ASSERT(arr[i] == i + 1, "arr[%d] must be %d, got %d", i, i + 1, arr[i]);

  // append range into NULL
  int* arr2 = NULL;
  int vals[] = {10, 20, 30};
  darr_append_range(arr2, vals, 3);
  TEST_ASSERT(darr_len(arr2) == 3, "Length must be 3 after append_range into NULL, got %zu",
              darr_len(arr2));
  TEST_ASSERT(arr2[0] == 10, "arr2[0] must be 10, got %d", arr2[0]);
}

void test_reserve() {
  TEST_SUITE("Reserve");

  int* arr = NULL;
  darr_init(arr);
  darr_reserve(arr, 64);

  TEST_ASSERT(darr_cap(arr) == 64, "Cap must be 64 after reserve, got %zu", darr_cap(arr));
  TEST_ASSERT(darr_len(arr) == 0, "Length must still be 0, got %zu", darr_len(arr));

  // elements must still be accessible after reserve
  for (int i = 0; i < 10; i++)
    darr_append(arr, i);

  TEST_ASSERT(darr_len(arr) == 10, "Length must be 10, got %zu", darr_len(arr));
  TEST_ASSERT(darr_cap(arr) == 64, "Cap must still be 64, got %zu", darr_cap(arr));
}

void test_clone() {
  TEST_SUITE("Clone");

  int* src = NULL;
  darr_append(src, 1);
  darr_append(src, 2);
  darr_append(src, 3);

  int* dst = NULL;
  darr_clone(src, dst);

  TEST_ASSERT(darr_len(dst) == darr_len(src), "Cloned length must match source, got %zu vs %zu",
              darr_len(dst), darr_len(src));

  for (size_t i = 0; i < darr_len(src); i++)
    TEST_ASSERT(dst[i] == src[i], "dst[%zu] must equal src[%zu]: %d vs %d", i, i, dst[i], src[i]);

  // mutating dst must not affect src
  dst[0] = 99;
  TEST_ASSERT(src[0] == 1, "src[0] must be unaffected by dst mutation, got %d", src[0]);
}

static int destructor_called = 0;

void int_destructor(int* val) {
  (void)val;
  destructor_called++;
}

void test_destructor() {
  TEST_SUITE("Destructor");

  destructor_called = 0;

  int* arr = NULL;
  darr_init(arr);
  darr_set_destructor(arr, int_destructor);

  darr_append(arr, 1);
  darr_append(arr, 2);
  darr_append(arr, 3);

  darr_pop(arr);
  TEST_ASSERT(destructor_called == 1, "Destructor must be called once on pop, called %d times",
              destructor_called);

  darr_remove_at(arr, 0);
  TEST_ASSERT(destructor_called == 2, "Destructor must be called on remove_at, called %d times",
              destructor_called);

  darr_clear(arr);
  TEST_ASSERT(destructor_called == 3,
              "Destructor must be called for each element on clear, called %d times",
              destructor_called);

  darr_append(arr, 10);
  darr_append(arr, 20);
  destructor_called = 0;
  darr_destroy(arr);
  TEST_ASSERT(destructor_called == 2,
              "Destructor must be called for each element on destroy, called %d times",
              destructor_called);
}

void test_clear() {
  TEST_SUITE("Clear");

  int* arr = NULL;
  darr_append(arr, 1);
  darr_append(arr, 2);
  darr_append(arr, 3);

  size_t cap_before = darr_cap(arr);
  darr_clear(arr);

  TEST_ASSERT(darr_len(arr) == 0, "Length must be 0 after clear, got %zu", darr_len(arr));
  TEST_ASSERT(darr_cap(arr) == cap_before, "Cap must be unchanged after clear, got %zu vs %zu",
              darr_cap(arr), cap_before);

  // must be usable after clear
  darr_append(arr, 42);
  TEST_ASSERT(darr_len(arr) == 1, "Must be usable after clear, length got %zu", darr_len(arr));
  TEST_ASSERT(arr[0] == 42, "arr[0] must be 42 after re-append, got %d", arr[0]);
}

int main(void) {
  test_init();
  test_append();
  test_resize();
  test_pop();
  test_set();
  test_insert_at();
  test_remove_at();
  test_append_range();
  test_reserve();
  test_clone();
  test_destructor();
  test_clear();
  test_arena_init_append();
  test_arena_reserve_safety();

  TEST_SUMMARY();
  return _test_fail_count > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
