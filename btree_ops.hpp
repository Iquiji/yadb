#ifndef _btree_ops_hpp_INCLUDED
#define _btree_ops_hpp_INCLUDED

#include "btree_base.hpp"
#include "btree_ext.hpp"
#include <cstdio>
#include <cstring>

void BTreeNodeInfo(BTreeNode *node);

bool BTDelete(BTree *tree, Bytes key);
void BTInsert(BTree *tree, Bytes key, Bytes val);
Bytes BTFind(BTree *tree, Bytes key);
#endif