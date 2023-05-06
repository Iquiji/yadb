#ifndef _btree_base_hpp_INCLUDED
#define _btree_base_hpp_INCLUDED

// #define DEBUG
// #define INFO

#include <cassert>
#include <cstdio>
#include <cstdlib>

const int HEADER = 4;
const int BTREE_PAGE_SIZE = 4096;
const int BTREE_MAX_KEY_SIZE = 1024;
const int BTREE_MAX_VAL_SIZE = 3000;

enum BTreeNodeTypes {
  BNODE_INVALID = 0,
  BNODE_NODE = 1,
  BNODE_LEAF = 2,
};

/*
A node consists of:
    1. A fixed-sized header containing the type of the node (leaf node or
internal node) and the number of keys.
    2. A list of pointers to the child nodes. (Used by internal nodes).
    3. A list of offsets pointing to each key-value pair.
    4. Packed KV pairs.

| type | nkeys | pointers   | offsets    | key-values
| 2B   | 2B    | nkeys * 8B | nkeys * 2B | ...

This is the format of the KV pair. Lengths followed by data.

| klen | vlen | key | val |
| 2B   | 2B   | ... | ... |

To keep things simple, both leaf nodes and internal nodes use the same format.
*/

struct BTreeNode {
  u_int64_t data[0];
};

struct Bytes {
  u_int16_t len;
  u_int8_t *data;
};

u_int16_t getBTtype(BTreeNode *node);
void setBTtype(BTreeNode *node, u_int16_t type);

u_int16_t getBTnum_keys(BTreeNode *node);
void setBTnum_keys(BTreeNode *node, u_int16_t num_keys);

u_int64_t getBTptr(BTreeNode *node, u_int16_t idx);
void setBTptr(BTreeNode *node, u_int16_t idx, u_int64_t ptr);

u_int16_t getBTOffset(BTreeNode *node, u_int16_t idx);
void setBTOffset(BTreeNode *node, u_int16_t idx, u_int16_t offset);

u_int16_t BTkvPos(BTreeNode *node, u_int16_t idx);
Bytes getBTKey(BTreeNode *node, u_int16_t idx);
Bytes getBTVal(BTreeNode *node, u_int16_t idx);

u_int16_t BTNodeSize(BTreeNode *node);

void BTinit();

#endif
