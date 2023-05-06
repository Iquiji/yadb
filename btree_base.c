#include "btree_base.h"

uint16_t getBTtype(BTreeNode *node) { return *(uint16_t *)((char *)node); }
void setBTtype(BTreeNode *node, uint16_t type) {
  *(uint16_t *)((char *)node) = type;
}

uint16_t getBTnum_keys(BTreeNode *node) {
  return *(uint16_t *)((char *)node + 2);
}
void setBTnum_keys(BTreeNode *node, uint16_t num_keys) {
  *(uint16_t *)((char *)node + 2) = num_keys;
}

uint64_t getBTptr(BTreeNode *node, uint16_t idx) {
  assert(idx < getBTnum_keys(node));
  uint64_t pos = HEADER + 8 * idx;
  return *(uint64_t *)((char *)node + pos);
}
void setBTptr(BTreeNode *node, uint16_t idx, uint64_t ptr) {
  assert(idx < getBTnum_keys(node));
  uint64_t pos = HEADER + 8 * idx;
  *(uint64_t *)((char *)node + pos) = ptr;
}

uint16_t getBTOffsetPos(BTreeNode *node, uint16_t idx) {
  if (idx == 0) {
    return 0;
  }
  return HEADER + 8 * getBTnum_keys(node) + 2 * (idx - 1);
}
uint16_t getBTOffset(BTreeNode *node, uint16_t idx) {
  return *(uint16_t *)((char *)node + getBTOffsetPos(node, idx));
}
void setBTOffset(BTreeNode *node, uint16_t idx, uint16_t offset) {
  *(uint16_t *)((char *)node + getBTOffsetPos(node, idx)) = offset;
}

uint16_t BTkvPos(BTreeNode *node, uint16_t idx) {
  return HEADER + 8 * getBTnum_keys(node) + 2 * getBTnum_keys(node) +
         getBTOffset(node, idx);
}
Bytes getBTKey(BTreeNode *node, uint16_t idx) {
  assert(idx < getBTnum_keys(node));
  uint16_t pos = BTkvPos(node, idx);
  uint16_t klen = *(uint16_t *)((char *)node + pos);

  Bytes bytes = {klen, (uint8_t *)((char *)node + pos + 4)};
  return bytes;
}

Bytes getBTVal(BTreeNode *node, uint16_t idx) {
  assert(idx < getBTnum_keys(node));
  uint16_t pos = BTkvPos(node, idx);
  uint16_t klen = *(uint16_t *)((char *)node + pos);
  uint16_t vlen = *(uint16_t *)((char *)node + pos + 2);

  Bytes bytes = {vlen, (uint8_t *)((char *)node + pos + 4 + klen)};
  return bytes;
}

uint16_t BTNodeSize(BTreeNode *node) {
  return BTkvPos(node, getBTnum_keys(node));
}

void BTinit() {
  int node1max = HEADER + 8 + 2 + 4 + BTREE_MAX_KEY_SIZE + BTREE_MAX_VAL_SIZE;
  assert(node1max <= BTREE_PAGE_SIZE);
}