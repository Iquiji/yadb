#ifndef _btree_ops_hpp_INCLUDED
#define _btree_ops_hpp_INCLUDED

#include "btree_base.h"
#include "btree_ext.h"
#include "stdio.h"
#include "string.h"

void BTreeNodeInfo(BTreeNode *node);

int BTDelete(BTree *tree, Bytes key);
void BTInsert(BTree *tree, Bytes key, Bytes val);
Bytes BTFind(BTree *tree, Bytes key);
#endif