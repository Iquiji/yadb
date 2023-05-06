#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "btree_base.hpp"
#include "btree_ops.hpp"

int main() {
  BTinit();

  BTreeNode *node = (BTreeNode *)malloc(42);

  setBTnum_keys(node, 65535);
  setBTptr(node, 0, 5);
  setBTptr(node, 1, 7);
  setBTptr(node, 2, 9);

  printf("%d \n", getBTnum_keys(node));
  printf("%lu \n", getBTptr(node, 0));
  printf("%lu \n", getBTptr(node, 1));
  printf("%lu \n", getBTptr(node, 2));

  return 0;
}