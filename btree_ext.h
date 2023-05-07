#ifndef _btree_ext_hpp_INCLUDED
#define _btree_ext_hpp_INCLUDED
#include "stdint.h"

#include "btree_base.h"

typedef struct {
  uint64_t root;
  BTreeNode *(*get_node)(uint64_t);
  uint64_t (*new_page)(BTreeNode *);
  void (*dealloc_page)(uint64_t);
} BTree;

#include "btree_ops.h"

typedef struct {
  BTree *tree;
} TestHarness;

void testBTree();
#endif