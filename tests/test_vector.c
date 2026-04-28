#include "../utils.h"
#include "../vector.h"
#include "assert.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct VEC_OF(int) Numbers;
typedef struct {
  char *key;
  Numbers value;
} NumbersHashMap;

void assert_el_equals(int *num, size_t idx) {
  TEST_ASSERT((int)idx == *num, "Element at idx %zu must be %d", idx, *num);
}

void test_append_items_correctly() {
  Numbers numbers = {0};
  vec_append(&numbers, &(int){0});
  vec_append(&numbers, &(int){1});
  vec_append(&numbers, &(int){2});
  vec_append(&numbers, &(int){3});
  vec_append(&numbers, &(int){4});

  TEST_ASSERT(5 == numbers.size, "The size of the vector must be equal to 5");
  TEST_ASSERT(8 == numbers.capacity,
              "The capacity of the vector must be equal to 8");

  vec_iterate_with_idx(&numbers, assert_el_equals);

  vec_destroy(&numbers);
}

void test_pop_items_correctly() {
  Numbers numbers = {0};
  vec_append(&numbers, &(int){0});
  vec_pop(&numbers);

  TEST_ASSERT(numbers.size == 0,
              "The vector size should go back to zero since we popped an item");

  vec_append(&numbers, &(int){1});
  vec_append(&numbers, &(int){2});
  vec_append(&numbers, &(int){3});
  vec_append(&numbers, &(int){4});

  vec_pop(&numbers);

  TEST_ASSERT(numbers.items[numbers.size - 1] == 3,
              "The last element of the vector must be equal to 3");

  vec_pop(&numbers);
  vec_pop(&numbers);

  TEST_ASSERT(
      numbers.capacity == MIN_DYNAMIC_ARRAY_SIZE,
      "The size of the array must have been reduced to the minimum array size");
}

void test_vector_set() {
  Numbers numbers = {0};

  vec_append(&numbers, &(int){0});
  vec_append(&numbers, &(int){1});
  vec_append(&numbers, &(int){2});

  vec_set(&numbers, 0, &(int){3});
  vec_set(&numbers, 1, &numbers.items[2]);

  TEST_ASSERT(numbers.items[0] == 3, "The first element should equal 3");
  TEST_ASSERT(
      numbers.items[1] == 2,
      "The second element should equal the value of the third element aka 2");
}

void test_insertion_at_specific_position() {
  Numbers numbers = {0};

  vec_insert_at(&numbers, 0, &(int){0});
  TEST_ASSERT(0 == numbers.items[0], "The first element must be 0");

  vec_append(&numbers, &(int){25});
  vec_append(&numbers, &(int){32});
  vec_append(&numbers, &(int){12});
  vec_append(&numbers, &(int){0});

  vec_insert_at(&numbers, 3, &(int){2});

  TEST_ASSERT(2 == numbers.items[3], "The fifth item must be equal to 2");
  TEST_ASSERT(12 == numbers.items[4], "The sixth item must be equal to 12");

  vec_destroy(&numbers);
}

void test_removal_at_specific_position() {
  Numbers numbers = {0};

  // Nothing should happen
  vec_remove_at(&numbers, 0);

  vec_append(&numbers, &(int){1});
  vec_append(&numbers, &(int){2});
  vec_append(&numbers, &(int){3});
  vec_append(&numbers, &(int){4});

  vec_remove_at(&numbers, 1);

  TEST_ASSERT(3 == numbers.items[1], "The second item must be equal to 3");
}

void test_append_range() {
  Numbers numbers = {0};

  int list[4] = {0, 1, 2, 3};

  vec_append_range(&numbers, list, 4);

  for (size_t i = 0; i < numbers.size; ++i) {
    int element = list[i];
    TEST_ASSERT(element == numbers.items[i],
                "The element at idx %zu must be %d after range insertion", i,
                element);
  }
}

void test_clone() {
  Numbers numbers = {0};
  Numbers target = {0};
  int list[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  vec_append_range(&numbers, list, 10);
  vec_clone(&numbers, &target);

  for (size_t i = 0; i < target.size; ++i) {
    int element = list[i];
    TEST_ASSERT(element == target.items[i],
                "The element at idx %zu must be %d after cloning", i, element);
  }
}

bool is_even(int *num) { return *num % 2 == 0; }

void test_filter() {
  Numbers numbers = {0};
  Numbers target = {0};

  int list[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  vec_append_range(&numbers, list, 10);
  vec_filter(&numbers, &target, is_even);

  for (size_t i = 0; i < target.size; ++i) {
    int element = list[i * 2];
    TEST_ASSERT(element == target.items[i],
                "The element at idx %zu must be %d after filtering", i,
                element);
  }
}

int main(int argc, char *argv[]) {
  test_append_items_correctly();
  test_pop_items_correctly();
  test_vector_set();
  test_insertion_at_specific_position();
  test_removal_at_specific_position();
  test_append_range();
  test_clone();
  test_filter();
  return EXIT_SUCCESS;
}
