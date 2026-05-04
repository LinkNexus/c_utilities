#include "../hashmap.h"
#include "../strv.h"
#include "../test_utils.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *name;
  short age;
} Student;

typedef struct HASHMAP_ENTRY_OF(char *) StringItem;
typedef struct HASHMAP_ENTRY_OF(size_t) NumItem;
typedef struct HASHMAP_ENTRY_OF(Student *) StudentsGroup;

void test_insertion() {
  TEST_SUITE("Insertion");

  StringItem *string_items = NULL;
  hashmap_set(string_items, "key1", "world");

  HashMapHdr *hdr = HASHMAP_HDR(string_items);
  size_t idx = hashmap_hash_key("key1") % hdr->cap;

  TEST_ASSERT(string_items[idx].key != NULL,
              "Entry at expected slot must not be NULL");
  TEST_ASSERT(strcmp(string_items[idx].key, "key1") == 0,
              "Key must match 'key1'");
  TEST_ASSERT(strcmp(string_items[idx].value, "world") == 0,
              "Value must match 'world'");
  TEST_ASSERT(hdr->size == 1, "Map size must be 1 after one insertion, got %zu",
              hdr->size);
}

void test_overwrite() {
  TEST_SUITE("Overwrite");

  StringItem *string_items = NULL;
  const char *key = "key1";

  hashmap_set(string_items, key, "Hello");
  hashmap_set(string_items, key, "World!");

  HashMapHdr *hdr = HASHMAP_HDR(string_items);
  size_t idx = hashmap_hash_key(key) % hdr->cap;

  TEST_ASSERT(strcmp(string_items[idx].value, "World!") == 0,
              "Value must be overwritten to 'World!'");
  TEST_ASSERT(hdr->size == 1,
              "Map size must still be 1 after overwrite, got %zu", hdr->size);
}

void test_getting_elements() {
  TEST_SUITE("Getting elements");

  StringItem *string_items = NULL;
  const char *key = "key1";
  hashmap_set(string_items, key, "Hello");

  HashMapHdr *hdr = HASHMAP_HDR(string_items);

  TEST_ASSERT(hashmap_get_idx(string_items, key) ==
                  hashmap_hash_key(key) % hdr->cap,
              "get_idx must return the correct slot");

  char **ref = NULL;
  TEST_ASSERT_FATAL(hashmap_get_ref(string_items, key, &ref),
                    "hashmap_get_ref must return true for existing key");
  TEST_ASSERT(ref != NULL, "ref must not be NULL after get_ref");
  TEST_ASSERT(strcmp(*ref, "Hello") == 0, "ref value must be 'Hello', got '%s'",
              *ref);

  *ref = "World!";

  char *copy = NULL;
  TEST_ASSERT_FATAL(hashmap_get(string_items, key, &copy),
                    "hashmap_get must return true for existing key");
  TEST_ASSERT(strcmp(copy, "World!") == 0,
              "Copied value must reflect mutation via ref, got '%s'", copy);

  char **missing = NULL;
  TEST_ASSERT(!hashmap_get_ref(string_items, "nonexistent", &missing),
              "hashmap_get_ref must return false for missing key");
}

void test_deleting_elements() {
  TEST_SUITE("Deleting elements");

  StringItem *string_items = NULL;
  hashmap_set(string_items, "key1", "Hello");
  hashmap_set(string_items, "key2", "World");

  HashMapHdr *hdr = HASHMAP_HDR(string_items);
  TEST_ASSERT(hdr->size == 2, "Size must be 2 before deletion, got %zu",
              hdr->size);

  hashmap_delete(string_items, "key1");

  TEST_ASSERT(hdr->size == 1, "Size must be 1 after deletion, got %zu",
              hdr->size);
  TEST_ASSERT(hdr->deleted_count == 1, "deleted_count must be 1, got %zu",
              hdr->deleted_count);

  char **value = NULL;
  TEST_ASSERT(!hashmap_get_ref(string_items, "key1", &value),
              "Deleted key must not be found");
  TEST_ASSERT(hashmap_get_ref(string_items, "key2", &value),
              "Non-deleted key must still be found");

  hashmap_delete(string_items, "nonexistent");
  TEST_ASSERT(hdr->size == 1,
              "Size must remain 1 after deleting nonexistent key, got %zu",
              hdr->size);
}

static int destructor_called = 0;

void free_string(char **str) {
  free(*str);
  destructor_called++;
}

void test_destructor() {
  TEST_SUITE("Destructor");

  destructor_called = 0;

  StringItem *string_items = NULL;
  hashmap_init(string_items);
  hashmap_set_destructor(string_items, free_string);

  char *value = strdup("Hello");
  hashmap_set(string_items, "key1", value);

  hashmap_delete(string_items, "key1");
  TEST_ASSERT(destructor_called == 1,
              "Destructor must be called once on delete, called %d times",
              destructor_called);

  char *value2 = strdup("World");
  hashmap_set(string_items, "key2", value2);
  hashmap_set(string_items, "key2", strdup("Overwritten"));
  TEST_ASSERT(destructor_called == 2,
              "Destructor must be called on overwrite too, called %d times",
              destructor_called);
}

void iterate_entries(StringItem *item, size_t idx) {
  printf("    [%zu] key='%s' value='%s'\n", idx, item->key, item->value);
}

void iterate_keys(const char *key, size_t idx) {
  printf("    [%zu] key='%s'\n", idx, key);
}

void iterate_values(char **value, size_t idx) {
  printf("    [%zu] value='%s'\n", idx, *value);
}

void test_iteration() {
  TEST_SUITE("Iteration");

  StringItem *string_items = NULL;
  hashmap_set(string_items, "key1", "Hello");
  hashmap_set(string_items, "key2", "World!");
  hashmap_set(string_items, "key3", "Foo");

  HashMapHdr *hdr = HASHMAP_HDR(string_items);
  TEST_ASSERT(hdr->size == 3, "Size must be 3, got %zu", hdr->size);

  printf("  iterate entries:\n");
  hashmap_iterate(string_items, iterate_entries);

  printf("  iterate keys:\n");
  hashmap_iterate_keys(string_items, iterate_keys);

  printf("  iterate values:\n");
  hashmap_iterate_values(string_items, iterate_values);
}

void test_resize() {
  TEST_SUITE("Resize / load factor");

  StringItem *string_items = NULL;

  // Insert enough to trigger at least one resize (cap starts at 4, LF=0.75)
  const char *keys[] = {"a", "b", "c", "d", "e", "f", "g", "h"};
  for (size_t i = 0; i < 8; i++)
    hashmap_set(string_items, keys[i], "val");

  HashMapHdr *hdr = HASHMAP_HDR(string_items);
  TEST_ASSERT(hdr->size == 8, "Size must be 8 after 8 insertions, got %zu",
              hdr->size);
  TEST_ASSERT(hdr->cap > 4, "Capacity must have grown beyond 4, got %zu",
              hdr->cap);

  // All keys must still be retrievable after resize
  for (size_t i = 0; i < 8; i++) {
    char **ref = NULL;
    TEST_ASSERT(hashmap_get_ref(string_items, keys[i], &ref),
                "Key '%s' must still be found after resize", keys[i]);
  }
}

// size_t convert_string_to_int(char **str, size_t idx) {
//   (void)idx;
//   return (size_t)strlen(*str);
// }
//
// void test_transform() {
//   TEST_SUITE("Transform");
//
//   StringItem *string_items = NULL;
//   hashmap_set(string_items, "hello", "Hi");
//   hashmap_set(string_items, "world", "Earth");
//
//   NumItem *num_items = NULL;
//   hashmap_transform(string_items, num_items, convert_string_to_int);
//
//   HashMapHdr *hdr = HASHMAP_HDR(num_items);
//   TEST_ASSERT(hdr->size == 2, "Transformed map must have 2 entries, got %zu",
//               hdr->size);
//
//   size_t val = 0;
//   TEST_ASSERT(hashmap_get(num_items, "hello", &val),
//               "Transformed key 'hello' must exist");
//   TEST_ASSERT(val == strlen("Hi"),
//               "Transformed value for 'hello' must be %zu, got %zu",
//               strlen("Hi"), val);
//
//   TEST_ASSERT(hashmap_get(num_items, "world", &val),
//               "Transformed key 'world' must exist");
//   TEST_ASSERT(val == strlen("Earth"),
//               "Transformed value for 'world' must be %zu, got %zu",
//               strlen("Earth"), val);
// }

int main(void) {
  test_insertion();
  test_overwrite();
  test_getting_elements();
  test_deleting_elements();
  test_destructor();
  test_iteration();
  test_resize();

  TEST_SUMMARY();
  return _test_fail_count > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
