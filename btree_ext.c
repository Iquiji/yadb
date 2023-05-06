// Hmmmmm
#include "btree_ext.h"

TestHarness *harness = 0;

BTreeNode *test_get_node(uint64_t page_ptr) {
#ifdef INFO
  printf("Get Node: %lu\n", page_ptr);
#endif
  // return harness->pages[page_ptr];
  return (BTreeNode *)page_ptr;
}

uint64_t test_new_page(BTreeNode *page_ptr) {
#ifdef INFO
  printf("New Page: %lu\n", page_ptr);
#endif
  // harness->pages[(uint64_t) page_ptr] = page_ptr;
  return (uint64_t)page_ptr;
}

void test_deallocate_page(uint64_t page_ptr) {
#ifdef INFO
  printf("Deallocate Page: %lu\n", page_ptr);
#endif
  // free((void*)page_ptr);
}

void initTestHarness() {
  harness = malloc(sizeof(TestHarness));
  harness->tree = (BTree *)malloc(sizeof(BTree));
  harness->tree->root = 0;
  harness->tree->get_node = test_get_node;
  harness->tree->new_page = test_new_page;
  harness->tree->dealloc_page = test_deallocate_page;
}

void add(char* key, char* val) {
  Bytes key_bytes = {(uint16_t)(((uint8_t)strlen(key) + 1)), (uint8_t *)key};
  Bytes val_bytes = {(uint16_t)(((uint8_t)strlen(val) + 1)), (uint8_t *)val};
  // printf("INSERTING: size: %d, string: %s\n", key_bytes.len, key_bytes.data);
  BTInsert(harness->tree, key_bytes, val_bytes);
}

char *find(char* key) {
  Bytes key_bytes = {(uint16_t)strlen(key), (uint8_t *)key};
  return (char *)BTFind(harness->tree, key_bytes).data;
}

int delete_(char* key) {
  Bytes key_bytes = {(uint16_t)strlen(key), (uint8_t *)key};
  return BTDelete(harness->tree, key_bytes);
}

void testBTree() {
  initTestHarness();
  printf("- Testing BTree:\n");
  add("hello", "world!");

  printf("hmmm\n");

  printf("find: %s\n", find("hello"));
  delete_("hello");

  add("Zhello1", "new world!!!!");
  add("1", "no!");
  add("hello", "world!");

  printf("find: %s\n", find("hello"));
  printf("find: %s\n", find("Zhello1"));
  printf("find: %s\n", find("1"));

  delete_("hello");
  printf("find: %s\n", find("hello"));
  add("hello", "world2!");
  printf("find: %s\n", find("hello"));
  printf("find: %s\n", find("Zhello1"));

  for (int i = 0; i < 10000; ++i) {
    // printf("root node size: %hu\n", BTNodeSize(harness->tree->get_node(harness->tree->root)));
    char string[] = "000000";
    string[5] = (char)(i % 10 + 48);
    string[4] = (char)((i % 100) / 10 + 48);
    string[3] = (char)((i % 1000) / 100 + 48);
    string[2] = (char)((i % 10000) / 1000 + 48);
    string[1] = (char)((i % 100000) / 10000 + 48);
    string[1] = (char)((i % 1000000) / 100000 + 48);

    add(string, "upupupdate!");
    assert(memcmp(find(string), "upupupdate!", 12) == 0);
    add(string, "0a1b2c3d4!");
    assert(memcmp(find(string), "0a1b2c3d4!", 11) == 0);
    delete_(string);
    assert(find(string) == 0);
    add(string, "upupupdate!");
  }
  printf("Test succesfully mastered!");
}