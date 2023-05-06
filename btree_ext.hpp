#ifndef _btree_ext_hpp_INCLUDED
#define _btree_ext_hpp_INCLUDED

#include "btree_base.hpp"

struct BTree {
  u_int64_t root;
  BTreeNode *(*get_node)(u_int64_t);
  u_int64_t (*new_page)(BTreeNode *);
  void (*dealloc_page)(u_int64_t);
};

#endif