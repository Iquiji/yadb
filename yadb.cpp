#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "btree_ext.hpp"

int main() {
  testBTree();

  // BTreeNode *node = (BTreeNode *)malloc(255);

  // setBTtype(node, 1);
  // setBTnum_keys(node, 65535);
  // setBTptr(node, 0, 5);
  // setBTptr(node, 1, 7);
  // setBTptr(node, 2, 9);
  // // *((u_int16_t*)node) = 1;
  // // *((u_int16_t*)node + 1) = 44;
  // printf("type: %hu\n", *((u_int16_t*)node));
  // printf("nkeys: %hu\n", *((u_int16_t*)((char*)node + 2)));

  // printf("%d \n", getBTnum_keys(node));
  // printf("%lu \n", getBTptr(node, 0));
  // printf("%lu \n", getBTptr(node, 1));
  // printf("%lu \n", getBTptr(node, 2));

  return 0;
}