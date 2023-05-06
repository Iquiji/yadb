#include "btree_ops.hpp"
#include <cstring>

u_int16_t BTNodeLookupLE(BTreeNode *node, Bytes key) {
  u_int16_t num_keys = getBTnum_keys(node);
  u_int16_t found = 0;

  for (int i = 0; i < num_keys; ++i) {
    int cmp = memcmp(getBTKey(node, i).data, key.data, key.len);

    if (cmp <= 0) {
      found = i;
    }
    if (cmp >= 0) {
      break;
    }
  }
  return found;
}

void BTNodeAppendRange(BTreeNode *old_node, BTreeNode *new_node,
                       u_int16_t dst_new, u_int16_t src_old, u_int16_t n) {
  assert(src_old + n < getBTnum_keys(old_node));
  assert(dst_new + n < getBTnum_keys(new_node));

  if (n == 0) {
    return;
  }

  // copy pointers
  for (int i = 0; i < n; ++i) {
    setBTptr(new_node, dst_new + i, getBTptr(old_node, src_old + i));
  }

  // copy offsets
  u_int16_t dst_begin = getBTOffset(new_node, dst_new);
  u_int16_t src_begin = getBTOffset(old_node, src_old);
  for (int i = 1; i <= n; ++i) {
    u_int16_t offset = dst_begin + getBTOffset(old_node, src_old + i) - src_begin;
    setBTOffset(new_node, dst_new + i, offset);
  }

  // copy KVs
  u_int16_t begin = BTkvPos(old_node, src_old);
  u_int16_t end = BTkvPos(old_node, src_old + n);
  memcpy(new_node->data + BTkvPos(new_node, dst_new), old_node->data, end - begin);
}

void BTNodeAppendKV(BTreeNode *node, u_int16_t idx, u_int64_t ptr, Bytes key, Bytes val) {
  // ptr
  setBTptr(node, idx, ptr);

  // KVs
  u_int16_t pos = BTkvPos(node, idx);
  *(u_int16_t *)(node->data + pos) = key.len;
  *(u_int16_t *)(node->data + pos + 2) = val.len;
  memcpy(node->data + pos + 4, key.data, key.len);
  memcpy(node->data + pos + 4 + key.len, val.data, val.len);

  // set offset
  setBTOffset(node, idx + 1, getBTOffset(node, idx) + 4 + key.len + val.len);
}

void BTleafInsert(BTreeNode *old_node, BTreeNode *new_node, u_int16_t idx,
                  Bytes key, Bytes val) {
  setBTtype(new_node, BNODE_LEAF);
  setBTnum_keys(new_node, getBTnum_keys(old_node) + 1);

  BTNodeAppendRange(old_node, new_node, 0, 0, idx);
  BTNodeAppendKV(new_node, idx, 0, key, val);
  BTNodeAppendRange(old_node, new_node, idx + 1, idx, getBTnum_keys(old_node) - idx);
}
void BTleafUpdate(BTreeNode *old_node, BTreeNode *new_node, u_int16_t idx,
                  Bytes key, Bytes val) {
  setBTtype(new_node, BNODE_LEAF);
  setBTnum_keys(new_node, getBTnum_keys(old_node) + 1);

  BTNodeAppendRange(old_node, new_node, 0, 0, idx);

  // update instead of insert
  u_int16_t pos = BTkvPos(old_node, idx);
  memcpy(new_node->data + pos + 4 + key.len, val.data, val.len);
  setBTOffset(new_node, idx + 1, getBTOffset(new_node, idx) + 4 + key.len + val.len);

  BTNodeAppendRange(old_node, new_node, idx, idx, getBTnum_keys(old_node) - idx);
}

void BTNodeInsert(BTree *tree, BTreeNode *new_node, BTreeNode *node, u_int16_t idx, Bytes key, Bytes val);

// insert a KV into a node, the result might be split into 2 nodes.
// the caller is responsible for deallocating the input node
// and splitting and allocating result nodes.
BTreeNode *BTreeInsert(BTree *tree, BTreeNode *node, Bytes key, Bytes val) {
  // alloc new Node, will be split if needed...
  BTreeNode *new_node = (BTreeNode *)malloc(2 * BTREE_PAGE_SIZE);

  // where to insert key?
  u_int16_t idx = BTNodeLookupLE(node, key);
  switch (getBTtype(node)) {
  case BNODE_INVALID:
    fprintf(stderr, "INVALID NODE: INSERT");
    exit(1);
    break;
  case BNODE_LEAF:
    if (memcmp(key.data, getBTKey(node, idx).data, key.len) == 0) {
      // found the key, update it...
      BTleafUpdate(node, new_node, idx, key, val);
    } else {
      // insert it after..
      BTleafInsert(node, new_node, idx + 1, key, val);
    }
    break;
  case BNODE_NODE:
    BTNodeInsert(tree, new_node, node, idx, key, val);
    break;
  }
  return new_node;
}

struct SplitHolder {
  u_int8_t size;
  BTreeNode *splitted[3];
};
SplitHolder BTNodeSplit3(BTreeNode *old_node);
void BTNodeReplaceKidN(BTree *tree, BTreeNode *new_node, BTreeNode *old_node, u_int16_t idx, SplitHolder split);

// part of the treeInsert(): KV insertion to an internal node
void BTNodeInsert(BTree *tree, BTreeNode *new_node, BTreeNode *node, u_int16_t idx, Bytes key, Bytes val) {
  // get and deallocate the kid node
  u_int64_t kid_ptr = getBTptr(node, idx);
  BTreeNode *kid_node = tree->get_node(kid_ptr);
  // FIXME:
  tree->dealloc_page(kid_ptr);

  // recursive insertion to the kid node
  kid_node = BTreeInsert(tree, kid_node, key, val);

  // split the result
  SplitHolder split = BTNodeSplit3(kid_node);

  // update the kid links
  BTNodeReplaceKidN(tree, new_node, node, idx, split);
}

// split a bigger-than-allowed node into two.
// the second node always fits on a page
void BTNodeSplit2(BTreeNode *left, BTreeNode *right, BTreeNode *old_node) {
  // code omitted...
  assert(getBTnum_keys(old_node) >= 2);

  // initial guess
  u_int16_t num_left = getBTnum_keys(old_node) / 2;
  while (HEADER + 8 * num_left + 2 * num_left + getBTOffset(old_node, num_left) > BTREE_PAGE_SIZE)
    --num_left;
  assert(num_left > 1);

  while (BTNodeSize(old_node) - HEADER + 8 * num_left + 2 * num_left + getBTOffset(old_node, num_left + HEADER) > BTREE_PAGE_SIZE)
    ++num_left;
  assert(num_left < getBTnum_keys(old_node));

  u_int16_t num_right = getBTnum_keys(old_node) - num_left;

  setBTtype(left, getBTtype(old_node));
  setBTtype(right, getBTtype(old_node));
  setBTnum_keys(left, num_left);
  setBTnum_keys(right, num_right);

  BTNodeAppendRange(old_node, left, 0, 0, num_left);
  BTNodeAppendRange(old_node, left, 0, num_left, num_right);

  assert(BTNodeSize(right) < BTREE_PAGE_SIZE);
}

// split a node if it's too big. the results are 1~3 nodes.
SplitHolder BTNodeSplit3(BTreeNode *old_node) {
  if (BTNodeSize(old_node) <= BTREE_PAGE_SIZE) {
    SplitHolder res = {1, {old_node}};
    return res;
  }

  BTreeNode *left = (BTreeNode *)malloc(2 * BTREE_PAGE_SIZE);
  BTreeNode *right = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
  BTNodeSplit2(left, right, old_node);

  if (BTNodeSize(left) <= BTREE_PAGE_SIZE) {
    SplitHolder res = {2, {left, right}};
    return res;
  }

  BTreeNode *leftleft = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
  BTreeNode *middle = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
  BTNodeSplit2(leftleft, middle, left);

  assert(BTNodeSize(leftleft) <= BTREE_PAGE_SIZE);

  SplitHolder res = {3, {leftleft, middle, right}};
  return res;
}

// replace a link with multiple links
void BTNodeReplaceKidN(BTree *tree, BTreeNode *new_node, BTreeNode *old_node, u_int16_t idx, SplitHolder split) {
  if (split.size == 1) {
    memcpy(new_node, old_node, BTNodeSize(old_node));
    setBTptr(new_node, idx, tree->new_page(split.splitted[0]));
    return;
  }

  setBTnum_keys(new_node, getBTnum_keys(old_node) + split.size - 1);
  BTNodeAppendRange(old_node, new_node, 0, 0, idx);
  for (int i = 0; i < split.size; ++i) {
    BTNodeAppendKV(new_node, idx + i, tree->new_page(split.splitted[i]), getBTKey(split.splitted[i], 0), Bytes{0, 0});
  }
  BTNodeAppendRange(old_node, new_node, idx + split.size, idx + 1, getBTnum_keys(old_node) - (idx + 1));
}

// remove a key from a leaf node
void BTLeafDelete(BTreeNode *new_node, BTreeNode *old_node, u_int16_t idx) {
  setBTnum_keys(new_node, getBTnum_keys(old_node) - 1);
  BTNodeAppendRange(old_node, new_node, 0, 0, idx);
  BTNodeAppendRange(old_node, new_node, idx, idx + 1, getBTnum_keys(old_node) - idx - 1);
}

struct ShouldMergeInfo {
  int8_t info;
  BTreeNode *sibling;
};

ShouldMergeInfo shouldMerge(BTree *tree, BTreeNode *node, u_int16_t idx, BTreeNode *updated_kid) {
  if (BTNodeSize(updated_kid) > BTREE_PAGE_SIZE / 4) {
    ShouldMergeInfo res = {0, 0};
    return res;
  }

  if (idx > 0) {
    BTreeNode *sibling = tree->get_node(getBTptr(node, idx - 1));
    u_int16_t merged_size = BTNodeSize(updated_kid) + BTNodeSize(sibling) - HEADER;
    if (merged_size <= BTREE_PAGE_SIZE) {
      ShouldMergeInfo res = {-1, sibling};
      return res;
    }
  }

  if (idx + 1 < getBTnum_keys(node)) {
    BTreeNode *sibling = tree->get_node(getBTptr(node, idx + 1));
    u_int16_t merged_size = BTNodeSize(updated_kid) + BTNodeSize(sibling) - HEADER;
    if (merged_size <= BTREE_PAGE_SIZE) {
      ShouldMergeInfo res = {+1, sibling};
      return res;
    }
  }

  ShouldMergeInfo res = {0, 0};
  return res;
}

BTreeNode *BTreeDelete(BTree *tree, BTreeNode *node, Bytes key);
void BTNodeMerge(BTreeNode *new_node, BTreeNode *left, BTreeNode *right) {
  setBTtype(new_node, getBTtype(left));
  setBTnum_keys(new_node, getBTnum_keys(left) + getBTnum_keys(right));
  BTNodeAppendRange(left, new_node, 0, 0, getBTnum_keys(left));
  BTNodeAppendRange(left, new_node, getBTnum_keys(left), 0, getBTnum_keys(right));
}

void BTNodeReplace2Kid(BTreeNode *new_node, BTreeNode *node, u_int16_t idx, u_int64_t merged_ptr, Bytes key) {
  setBTtype(new_node, getBTtype(node));
  setBTnum_keys(new_node, getBTnum_keys(node) - 1);

  BTNodeAppendRange(node, new_node, 0, 0, idx);
  BTNodeAppendKV(new_node, idx, merged_ptr, key, Bytes{0, 0});
  BTNodeAppendRange(node, new_node, idx + 1, idx + 2, getBTnum_keys(node) - idx - 2);
}

// part of BTreeDelete
BTreeNode *BTNodeDelete(BTree *tree, BTreeNode *node, u_int16_t idx, Bytes key) {
  u_int64_t kid_ptr = getBTptr(node, idx);
  BTreeNode *updated = BTreeDelete(tree, tree->get_node(kid_ptr), key);

  // not found
  if (!updated) {
    return 0;
  }
  tree->dealloc_page(kid_ptr);

  BTreeNode *new_node = (BTreeNode *)malloc(BTREE_PAGE_SIZE);

  ShouldMergeInfo merge_info = shouldMerge(tree, node, idx, updated);
  if (merge_info.info == -1) {

    BTreeNode *merged = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
    BTNodeMerge(merged, merge_info.sibling, updated);
    tree->dealloc_page(getBTptr(node, idx - 1));
    BTNodeReplace2Kid(new_node, node, idx - 1, tree->new_page(merged), getBTKey(merged, 0));
  } else if (merge_info.info == 1) {
    BTreeNode *merged = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
    BTNodeMerge(merged, merge_info.sibling, updated);
    tree->dealloc_page(getBTptr(node, idx + 1));
    BTNodeReplace2Kid(new_node, node, idx, tree->new_page(merged), getBTKey(merged, 0));
  } else {
    assert(getBTnum_keys(updated) > 0);
    SplitHolder split = {1, {updated}};
    BTNodeReplaceKidN(tree, new_node, node, idx, split);
  }
  return new_node;
}

// delete a key from the tree
BTreeNode *BTreeDelete(BTree *tree, BTreeNode *node, Bytes key) {
  u_int16_t idx = BTNodeLookupLE(node, key);

  switch (getBTtype(node)) {
  case BNODE_INVALID:
    fprintf(stderr, "INVALID NODE: DELETE");
    exit(1);
    break;
  case BNODE_LEAF:
    if (memcmp(key.data, getBTKey(node, idx).data, key.len) == 0) {
      // found the key, update it...
      BTreeNode *new_node = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
      BTLeafDelete(new_node, node, idx);
      return new_node;
    } else {
      // Not found!
      return 0;
    }
    break;
  case BNODE_NODE:
    return BTNodeDelete(tree, node, idx, key);
    break;
  }

  return 0;
}

bool BTDelete(BTree *tree, Bytes key) {
  assert(key.len != 0);
  assert(key.len <= BTREE_MAX_KEY_SIZE);

  if (tree->root == 0) {
    return false;
  }

  BTreeNode *updated = BTreeDelete(tree, tree->get_node(tree->root), key);
  if (updated == 0) {
    return 0;
  }

  tree->dealloc_page(tree->root);
  if (getBTtype(updated) == BNODE_NODE && getBTnum_keys(updated) == 1) {
    // remove a level
    tree->root = getBTptr(updated, 0);
  } else {
    tree->root = tree->new_page(updated);
  }

  return true;
}

void BTInsert(BTree *tree, Bytes key, Bytes val) {
  assert(key.len != 0);
  assert(key.len <= BTREE_MAX_KEY_SIZE);
  assert(val.len <= BTREE_MAX_KEY_SIZE);

  if (tree->root == 0) {
    // create root
    BTreeNode *new_root = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
    setBTtype(new_root, BNODE_LEAF);
    setBTnum_keys(new_root, 2);

    // a dummy key, this makes the tree cover the whole key space.
    // thus a lookup can always find a containing node.
    BTNodeAppendKV(new_root, 0, 0, Bytes{0, 0}, Bytes{0, 0});
    BTNodeAppendKV(new_root, 1, 0, key, val);
    tree->root = tree->new_page(new_root);
    return;
  }

  BTreeNode *root = tree->get_node(tree->root);
  tree->dealloc_page(tree->root);

  root = BTreeInsert(tree, root, key, val);
  SplitHolder split = BTNodeSplit3(root);
  if (split.size > 1) {
    BTreeNode *new_root = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
    setBTtype(new_root, BNODE_NODE);
    setBTnum_keys(new_root, split.size);
    for (int i = 0; i < split.size; ++i) {
      BTNodeAppendKV(new_root, i, tree->new_page(split.splitted[i]), getBTKey(split.splitted[i], 0), Bytes{0, 0});
    }
    tree->root = tree->new_page(new_root);
  } else {
    tree->root = tree->new_page(split.splitted[0]);
  }
}

Bytes BTreeFind(BTree *tree, BTreeNode *node, Bytes key) {
  u_int16_t idx = BTNodeLookupLE(node, key);

  switch (getBTtype(node)) {
  case BNODE_INVALID:
    fprintf(stderr, "INVALID NODE: DELETE");
    exit(1);
    break;
  case BNODE_LEAF:
    if (memcmp(key.data, getBTKey(node, idx).data, key.len) == 0) {
      // found the key
      return getBTVal(node, idx);
    } else {
      // Not found!
      return Bytes{0, 0};
    }
    break;
  case BNODE_NODE:
    return BTreeFind(tree, tree->get_node(getBTptr(node, idx)), key);
    break;
  }

  return Bytes{0, 0};
}

Bytes BTFind(BTree *tree, Bytes key) {
  return BTreeFind(tree, tree->get_node(tree->root), key);
}