#ifndef _btree_ext_hpp_INCLUDED
#define _btree_ext_hpp_INCLUDED

#include "btree_base.hpp"

struct BTree {
  u_int64_t root = 0;
  BTreeNode *(*get_node)(u_int64_t);
  u_int64_t (*new_page)(BTreeNode *);
  void (*dealloc_page)(u_int64_t);
};

#include "btree_ops.hpp"
#include <iostream>
#include <map>


class TestHarness{
public:
    BTree *tree = 0;
    std::map<std::string, std::string> reference_map;
    std::map<u_int64_t, BTreeNode*> pages;
};


void testBTree();
#endif