#include "btree_ops.hpp"
#include <cstring>

void BTreeNodeInfo(BTreeNode *node){
  #ifdef DEBUG
  // printf("INFO BTreeNode:\n");
  // printf(" - type: %d\n",getBTtype(node));
  // printf(" - nkeys: %d\n",getBTnum_keys(node));
  for (int i = 0; i < getBTnum_keys(node); ++i){
    // printf("  - Pointer: %lx\n",getBTptr(node,i));
    // printf("  - Offset: %d\n",getBTOffset(node,i));
    u_int16_t pos = BTkvPos(node,i);
    u_int16_t key_len = *(u_int16_t*) ((char*)node + pos);
    u_int16_t val_len = *(u_int16_t*) ((char*)node + pos + 2);
    // printf("  - key len: %d, data: '", key_len);
    fwrite((char*)node + pos + 4, 1, key_len,  stdout);
    // printf("'\n");
    // printf("  - val len: %d, data: '", val_len);
    fwrite((char*)node + pos + key_len + 4, 1, val_len,  stdout);
    // printf("'\n\n");
  }
  #endif
}

u_int16_t BTNodeLookupLE(BTreeNode *node, Bytes key) {
  u_int16_t num_keys = getBTnum_keys(node);
  u_int16_t found = 0;

  for (int i = 0; i < num_keys; ++i) {
    if (getBTKey(node, i).len == 0){
      continue;
    }

    // if differently dized take minimum length
    u_int16_t cmp_len = (getBTKey(node, i).len < key.len) ? getBTKey(node, i).len : key.len;

    int cmp = memcmp(getBTKey(node, i).data, key.data, cmp_len);
    
    // printf("cmp: %d\n", cmp);

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
  // printf("getBTnum_keys(old_node): %d\n",getBTnum_keys(old_node));
  // printf("getBTnum_keys(new_node): %d\n",getBTnum_keys(new_node));
  // printf("n: %d\n",n);
  assert(src_old + n <= getBTnum_keys(old_node));
  assert(dst_new + n <= getBTnum_keys(new_node));

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
  // printf("begin: %hu, end: %hu\n", begin, end);
  if (end < begin){
    // printf("!!! end < begin !!!\n");
    return;
  }
  // for (int i = 0; i < end - begin; ++i){
    // printf("%hhu ", (char*)old_node + begin + i);
  // }
  memcpy((char*)new_node + BTkvPos(new_node, dst_new), (char*)old_node + begin, end - begin);
}

void BTNodeAppendKV(BTreeNode *node, u_int16_t idx, u_int64_t ptr, Bytes key, Bytes val) {
  // ptr
  setBTptr(node, idx, ptr);

  // KVs
  u_int16_t pos = BTkvPos(node, idx);
  *(u_int16_t *)((char*)node + pos) = key.len;
  *(u_int16_t *)((char*)node + pos + 2) = val.len;
  memcpy((char*)node + pos + 4, key.data, key.len);
  memcpy((char*)node + pos + 4 + key.len, val.data, val.len);

  // set offset
  setBTOffset(node, idx + 1, getBTOffset(node, idx) + 4 + key.len + val.len);
}

void BTleafInsert(BTreeNode *old_node, BTreeNode *new_node, u_int16_t idx,
                  Bytes key, Bytes val) {
  setBTtype(new_node, BNODE_LEAF);
  setBTnum_keys(new_node, getBTnum_keys(old_node) + 1);

  BTNodeAppendRange(old_node, new_node, 0, 0, idx);
  BTreeNodeInfo(new_node);
  BTNodeAppendKV(new_node, idx, 0, key, val);
  BTNodeAppendRange(old_node, new_node, idx + 1, idx, getBTnum_keys(old_node) - idx);
}
void BTleafUpdate(BTreeNode *old_node, BTreeNode *new_node, u_int16_t idx,
                  Bytes key, Bytes val) {
  setBTtype(new_node, BNODE_LEAF);
  setBTnum_keys(new_node, getBTnum_keys(old_node));

  BTNodeAppendRange(old_node, new_node, 0, 0, idx);

  BTNodeAppendKV(new_node, idx, 0, key, val);
  BTNodeAppendRange(old_node, new_node, idx + 1, idx + 1, getBTnum_keys(old_node) - idx - 1);
}

void BTNodeInsert(BTree *tree, BTreeNode *new_node, BTreeNode *node, u_int16_t idx, Bytes key, Bytes val);

// insert a KV into a node, the result might be split into 2 nodes.
// the caller is responsible for deallocating the input node
// and splitting and allocating result nodes.
BTreeNode *BTreeInsert(BTree *tree, BTreeNode *node, Bytes key, Bytes val) {
  // alloc new Node, will be split if needed...
  BTreeNode *new_node = (BTreeNode *)malloc(2 * BTREE_PAGE_SIZE);
  memset(new_node, 0, 2 * BTREE_PAGE_SIZE);

  // printf("node addr: %lx\n", (void*)node);
  // printf("malloc addr: %lx\n", (void*)new_node);

  // where to insert key?
  u_int16_t idx = BTNodeLookupLE(node, key);
  // printf("Insert idx: %hu\n",idx);
  BTreeNodeInfo(node);
  switch (getBTtype(node)) {
  case BNODE_INVALID:
    fprintf(stderr, "INVALID NODE: INSERT\n");
    exit(1);
    break;
  case BNODE_LEAF:
    if (memcmp(key.data, getBTKey(node, idx).data, key.len) == 0) {
      // found the key, update it...
      BTleafUpdate(node, new_node, idx, key, val);
      // printf("exited after match on: %s\n", key.data);
      // BTreeNodeInfo(new_node);
      // exit(0);
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
  // printf("size %hu\n", BTNodeSize(old_node));

  // initial guess
  u_int16_t num_left = getBTnum_keys(old_node) / 2;
  while (HEADER + 8 * num_left + 2 * num_left + getBTOffset(old_node, num_left) > BTREE_PAGE_SIZE)
    --num_left;
  assert(num_left > 1);

  // printf("num_left %hu\n", num_left);

  while ((BTNodeSize(old_node) - (8 * num_left + 2 * num_left + getBTOffset(old_node, num_left))) > BTREE_PAGE_SIZE){
    ++num_left;
    // if (num_left > getBTnum_keys(old_node))  {
    //   num_left = 1;
    // }
    // printf("num_left %hu, %hu\n", num_left, (BTNodeSize(old_node) - HEADER + 8 * num_left + 2 * num_left + getBTOffset(old_node, num_left) + HEADER));
  }
  assert(num_left < getBTnum_keys(old_node));

  u_int16_t num_right = getBTnum_keys(old_node) - num_left;
  // printf("num_right %hu\n", num_right);

  setBTtype(left, getBTtype(old_node));
  setBTtype(right, getBTtype(old_node));
  setBTnum_keys(left, num_left);
  setBTnum_keys(right, num_right);

  BTNodeAppendRange(old_node, left, 0, 0, num_left);
  BTNodeAppendRange(old_node, right, 0, num_left, num_right);

  assert(BTNodeSize(right) < BTREE_PAGE_SIZE);
}

// split a node if it's too big. the results are 1~3 nodes.
SplitHolder BTNodeSplit3(BTreeNode *old_node) {
  // printf("Splitting Node:\n");
  if (BTNodeSize(old_node) <= BTREE_PAGE_SIZE) {
    SplitHolder res = {1, {old_node}};
    return res;
  }

  BTreeNode *left = (BTreeNode *)malloc(2 * BTREE_PAGE_SIZE);
  memset(left, 0, 2 * BTREE_PAGE_SIZE);
  BTreeNode *right = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
  memset(right, 0, BTREE_PAGE_SIZE);
  BTNodeSplit2(left, right, old_node);

  if (BTNodeSize(left) <= BTREE_PAGE_SIZE) {
    SplitHolder res = {2, {left, right}};
    return res;
  }

  BTreeNode *leftleft = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
  memset(leftleft, 0, BTREE_PAGE_SIZE);
  BTreeNode *middle = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
  memset(middle, 0, BTREE_PAGE_SIZE);
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

  setBTtype(new_node, getBTtype(old_node));
  setBTnum_keys(new_node, getBTnum_keys(old_node) + split.size - 1);

  BTNodeAppendRange(old_node, new_node, 0, 0, idx);
  for (int i = 0; i < split.size; ++i) {
    BTNodeAppendKV(new_node, idx + i, tree->new_page(split.splitted[i]), getBTKey(split.splitted[i], 0), Bytes{0, 0});
  }
  BTNodeAppendRange(old_node, new_node, idx + split.size, idx + 1, getBTnum_keys(old_node) - (idx + 1));
}

// remove a key from a leaf node
void BTLeafDelete(BTreeNode *new_node, BTreeNode *old_node, u_int16_t idx) {
  // printf("BTLeafDelete %hu\n", idx);
  setBTtype(new_node, getBTtype(old_node));
  setBTnum_keys(new_node, getBTnum_keys(old_node) - 1);
  BTreeNodeInfo(old_node);
  BTreeNodeInfo(new_node);
  // printf("first\n");
  BTNodeAppendRange(old_node, new_node, 0, 0, idx);
  // printf("second\n");
  BTNodeAppendRange(old_node, new_node, idx, idx + 1, getBTnum_keys(old_node) - idx - 1);
  // printf("after second copy: \n");
  BTreeNodeInfo(new_node);
  // printf("end\n\n\n\n");
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
  memset(new_node, 0, BTREE_PAGE_SIZE);

  ShouldMergeInfo merge_info = shouldMerge(tree, node, idx, updated);
  if (merge_info.info == -1) {

    BTreeNode *merged = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
    memset(merged, 0, BTREE_PAGE_SIZE);

    BTNodeMerge(merged, merge_info.sibling, updated);
    tree->dealloc_page(getBTptr(node, idx - 1));
    BTNodeReplace2Kid(new_node, node, idx - 1, tree->new_page(merged), getBTKey(merged, 0));
  } else if (merge_info.info == 1) {
    BTreeNode *merged = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
    memset(merged, 0, BTREE_PAGE_SIZE);

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

  // printf("DELETE idx: %hu\n", idx);

  switch (getBTtype(node)) {
  case BNODE_INVALID:
    fprintf(stderr, "INVALID NODE: DELETE\n");
    exit(1);
    break;
  case BNODE_LEAF:
    if (memcmp(key.data, getBTKey(node, idx).data, key.len) == 0) {
      // found the key, update it...
      // printf("hmmm:\n");
      BTreeNode *new_node = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
      memset(new_node, 0, BTREE_PAGE_SIZE);
      
      BTLeafDelete(new_node, node, idx);
      BTreeNodeInfo(new_node);
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

  // printf("DELETING %s\n", key.data);
  BTreeNodeInfo(tree->get_node(tree->root));

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

  // printf("tree->root: %lx\n", tree->root);

  if (tree->root == 0) {
    // create root
    BTreeNode *new_root = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
    memset(new_root, 0, BTREE_PAGE_SIZE);
    setBTtype(new_root, BNODE_LEAF);
    setBTnum_keys(new_root, 2);

    // a dummy key, this makes the tree cover the whole key space.
    // thus a lookup can always find a containing node.
    BTNodeAppendKV(new_root, 0, 0, Bytes{1, (u_int8_t*)""}, Bytes{1, (u_int8_t*)""});
    BTNodeAppendKV(new_root, 1, 0, key, val);
    tree->root = tree->new_page(new_root);
    BTreeNodeInfo(new_root);
    return;
  }

  BTreeNode *root = tree->get_node(tree->root);
  tree->dealloc_page(tree->root);

  root = BTreeInsert(tree, root, key, val);
  SplitHolder split = BTNodeSplit3(root);
  if (split.size > 1) {
    BTreeNode *new_root = (BTreeNode *)malloc(BTREE_PAGE_SIZE);
    memset(new_root, 0, BTREE_PAGE_SIZE);

    setBTtype(new_root, BNODE_NODE);
    setBTnum_keys(new_root, split.size);
    for (int i = 0; i < split.size; ++i) {
      BTNodeAppendKV(new_root, i, tree->new_page(split.splitted[i]), getBTKey(split.splitted[i], 0), Bytes{0, 0});
    }
    tree->root = tree->new_page(new_root);
  } else {
    tree->root = tree->new_page(split.splitted[0]);
  }
  BTreeNodeInfo(root);
}

Bytes BTreeFind(BTree *tree, BTreeNode *node, Bytes key) {
  BTreeNodeInfo(node);
  u_int16_t idx = BTNodeLookupLE(node, key);

  switch (getBTtype(node)) {
  case BNODE_INVALID:
    fprintf(stderr, "INVALID NODE: FIND\n");
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