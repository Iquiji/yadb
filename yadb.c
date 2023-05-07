#include "assert.h"
#include "stdio.h"
#include "stdlib.h"

#include "kv_store.h"

void test_add(char *key, char *val) {
  Bytes key_bytes = {(uint16_t)(((uint8_t)strlen(key) + 1)), (uint8_t *)key};
  Bytes val_bytes = {(uint16_t)(((uint8_t)strlen(val) + 1)), (uint8_t *)val};
  // printf("INSERTING: size: %d, string: %s\n", key_bytes.len, key_bytes.data);
  kvSet(key_bytes, val_bytes);
}

char *test_find(char *key) {
  Bytes key_bytes = {(uint16_t)strlen(key), (uint8_t *)key};
  return (char *)kvGet(key_bytes).data;
}

int test_delete_(char *key) {
  Bytes key_bytes = {(uint16_t)strlen(key), (uint8_t *)key};
  return kvDelete(key_bytes);
}

const char* path_string = "./testdb.bin";

void testKV() {
  printf("- Testing KV:\n");
  printf("path_string: '%s'\n", path_string);
  printf("global_kv.path: '%s'\n", global_kv.path);
  kvInit();
  global_kv.path = path_string;
  kvOpen();
  test_add("hello", "world!");

  printf("hmmm\n");

  printf("find: %s\n", test_find("hello"));
  test_delete_("hello");

  test_add("Zhello1", "new world!!!!");
  test_add("1", "no!");
  test_add("hello", "world!");

  printf("find: %s\n", test_find("hello"));
  printf("find: %s\n", test_find("Zhello1"));
  printf("find: %s\n", test_find("1"));

  test_delete_("hello");
  printf("find: %s\n", test_find("hello"));
  test_add("hello", "world2!");
  printf("find: %s\n", test_find("hello"));
  printf("find: %s\n", test_find("Zhello1"));

  for (int i = 0; i < 10000; ++i) {
    // printf("root node size: %hu\n", BTNodeSize(harness->tree->get_node(harness->tree->root)));
    char string[] = "000000";
    string[5] = (char)(i % 10 + 48);
    string[4] = (char)((i % 100) / 10 + 48);
    string[3] = (char)((i % 1000) / 100 + 48);
    string[2] = (char)((i % 10000) / 1000 + 48);
    string[1] = (char)((i % 100000) / 10000 + 48);
    string[1] = (char)((i % 1000000) / 100000 + 48);

    test_add(string, "upupupdate!");
    assert(memcmp(test_find(string), "upupupdate!", 12) == 0);
    test_add(string, "0a1b2c3d4!");
    assert(memcmp(test_find(string), "0a1b2c3d4!", 11) == 0);
    test_delete_(string);
    assert(test_find(string) == 0);
    test_add(string, "upupupdate!");
  }
  printf("Test succesfully mastered!");

  kvClose();
}

int main() {
  testKV();

  return 0;
}