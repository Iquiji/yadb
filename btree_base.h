#ifndef _btree_base_hpp_INCLUDED
#define _btree_base_hpp_INCLUDED

// #define DEBUG
// #define INFO

#include "assert.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

#define HEADER 4
#define BTREE_PAGE_SIZE 4096
#define BTREE_MAX_KEY_SIZE 1024
#define BTREE_MAX_VAL_SIZE 3000

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

typedef struct {
  uint64_t data[0];
} BTreeNode;

typedef struct {
  uint16_t len;
  uint8_t *data;
} Bytes;

uint16_t getBTtype(BTreeNode *node);
void setBTtype(BTreeNode *node, uint16_t type);

uint16_t getBTnum_keys(BTreeNode *node);
void setBTnum_keys(BTreeNode *node, uint16_t num_keys);

uint64_t getBTptr(BTreeNode *node, uint16_t idx);
void setBTptr(BTreeNode *node, uint16_t idx, uint64_t ptr);

uint16_t getBTOffset(BTreeNode *node, uint16_t idx);
void setBTOffset(BTreeNode *node, uint16_t idx, uint16_t offset);

uint16_t BTkvPos(BTreeNode *node, uint16_t idx);
Bytes getBTKey(BTreeNode *node, uint16_t idx);
Bytes getBTVal(BTreeNode *node, uint16_t idx);

uint16_t BTNodeSize(BTreeNode *node);

void BTinit();

#endif
