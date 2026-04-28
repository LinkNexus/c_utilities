#include "../hashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct HASHENTRY_OF(char *) Item;

void test_insertion() {
  Item *items = NULL;

  hashmap_set(items, "key1", &(char *){"world"});

  HashMapHdr *hdr = HASHMAP_HDR(items);
  size_t idx = hashmap_hash_key("key1") % hdr->cap;

  TEST_ASSERT(strcmp(items[idx].key, "key1") == 0,
              "The key of the element must match");
  TEST_ASSERT(strcmp(items[idx].value, "world") == 0,
              "The value of the element must match");
}

void test_overwrite() {
  Item *items = NULL;

  const char *key = "key1";

  hashmap_set(items, key, &(char *){"Hello"});
  hashmap_set(items, key, &(char *){"World!"});

  HashMapHdr *hdr = HASHMAP_HDR(items);
  size_t idx = hashmap_hash_key(key) % hdr->cap;

  TEST_ASSERT(strcmp(items[idx].value, "World!") == 0,
              "The value of the element must match");
}

void test_getting_elements() {
  Item *items = NULL;
  const char *key = "key1";

  hashmap_set(items, key, &(char *){"Hello"});

  HashMapHdr *hdr = HASHMAP_HDR(items);
  TEST_ASSERT(hashmap_get_idx(items, key) == hashmap_hash_key(key) % hdr->cap,
              "The indexes must match");

  char **value;
  TEST_ASSERT(hashmap_get_ref(items, "key1", value),
              "The fetching of the element must work");
  TEST_ASSERT(strcmp(*value, "Hello") == 0,
              "The value of the output must match that of the entry");

  *value = "World!";
  char *new_val;
  TEST_ASSERT(hashmap_get(items, "key1", new_val),
              "The fetching (copy) of the element must be successfull");
  TEST_ASSERT(
      strcmp(*value, new_val) == 0,
      "The value of new_val must correspond to the new value of `value`");
}

void test_deleting_elements() {
  Item *items = NULL;

  hashmap_set(items, "key1", "Hello");
  hashmap_delete(items, "key1");

  char **value;
  TEST_ASSERT(!hashmap_get_ref(items, "key1", value),
              "The element with this key must not be fount in the hash map");
}

void free_string(char **str) { free(*str); }

void test_destructor() {
  Item *items = NULL;

  hashmap_init(items);
  hashmap_set_destructor(items, free_string);

  char *value = strdup("Hello");
  hashmap_set(items, "key1", &value);
  hashmap_delete(items, "key1");
}

void iterate_entries(Item *item, size_t idx) {
  printf("Entry at idx %zu with key %s and value %s\n", idx, item->key,
         item->value);
}

void iterate_keys(const char *key, size_t idx) {
  printf("Entry at idx %zu with key %s\n", idx, key);
}

void iterate_values(const char **value, size_t idx) {
  printf("Entry at idx %zu with key %s\n", idx, *value);
}

void test_iteration() {
  Item *items = NULL;

  hashmap_set(items, "key1", &(char *){"Hello"});
  hashmap_set(items, "key2", &(char *){"World!"});

  hashmap_iterate(items, iterate_entries);
  hashmap_iterate_keys(items, iterate_keys);
  hashmap_iterate_values(items, iterate_values);
}

int main(int argc, char *argv[]) {
  test_insertion();
  test_overwrite();
  test_getting_elements();
  test_deleting_elements();
  test_destructor();
  test_iteration();
  return EXIT_SUCCESS;
}
