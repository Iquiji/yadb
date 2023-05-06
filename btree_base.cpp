#include "btree_base.hpp"

u_int16_t getBTtype(BTreeNode *node) { return *(u_int16_t *)((char*)node); }
void setBTtype(BTreeNode *node, u_int16_t type) {
  *(u_int16_t *)((char*)node) = type;
}

u_int16_t getBTnum_keys(BTreeNode *node) {
  return *(u_int16_t *)((char*)node + 2);
}
void setBTnum_keys(BTreeNode *node, u_int16_t num_keys) {
  *(u_int16_t *)((char*)node + 2) = num_keys;
}

u_int64_t getBTptr(BTreeNode *node, u_int16_t idx) {
  assert(idx < getBTnum_keys(node));
  u_int64_t pos = HEADER + 8 * idx;
  return *(u_int64_t *)((char*)node + pos);
}
void setBTptr(BTreeNode *node, u_int16_t idx, u_int64_t ptr) {
  assert(idx < getBTnum_keys(node));
  u_int64_t pos = HEADER + 8 * idx;
  *(u_int64_t *)((char*)node + pos) = ptr;
}

u_int16_t getBTOffsetPos(BTreeNode *node, u_int16_t idx) {
  if (idx == 0) {
    return 0;
  }
  return HEADER + 8 * getBTnum_keys(node) + 2 * (idx - 1);
}
u_int16_t getBTOffset(BTreeNode *node, u_int16_t idx) {
  return *(u_int16_t *)((char*)node + getBTOffsetPos(node, idx));
}
void setBTOffset(BTreeNode *node, u_int16_t idx, u_int16_t offset) {
  *(u_int16_t *)((char*)node + getBTOffsetPos(node, idx)) = offset;
}

u_int16_t BTkvPos(BTreeNode *node, u_int16_t idx) {
  return HEADER + 8 * getBTnum_keys(node) + 2 * getBTnum_keys(node) +
         getBTOffset(node, idx);
}
Bytes getBTKey(BTreeNode *node, u_int16_t idx) {
  assert(idx < getBTnum_keys(node));
  u_int16_t pos = BTkvPos(node, idx);
  u_int16_t klen = *(u_int16_t *)((char*)node + pos);

  Bytes bytes = Bytes{klen, (u_int8_t *)((char*)node + pos + 4)};
  return bytes;
}

Bytes getBTVal(BTreeNode *node, u_int16_t idx) {
  assert(idx < getBTnum_keys(node));
  u_int16_t pos = BTkvPos(node, idx);
  u_int16_t klen = *(u_int16_t *)((char*)node + pos);
  u_int16_t vlen = *(u_int16_t *)((char*)node + pos + 2);

  Bytes bytes = Bytes{vlen, (u_int8_t *)((char*)node + pos + 4 + klen)};
  return bytes;
}

u_int16_t BTNodeSize(BTreeNode *node) {
  return BTkvPos(node, getBTnum_keys(node));
}

void BTinit() {
  int node1max = HEADER + 8 + 2 + 4 + BTREE_MAX_KEY_SIZE + BTREE_MAX_VAL_SIZE;
  assert(node1max <= BTREE_PAGE_SIZE);
}