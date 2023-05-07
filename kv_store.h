#ifndef _kv_store_hpp_INCLUDED
#define _kv_store_hpp_INCLUDED
#include "btree_ext.h"
#include "stdio.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
  uint64_t size;
  char *bytes;
} MMapResult;

typedef struct {
  char *path;

  FILE *fp;
  int fd;
  BTree *tree;

  struct {
    int file;
    int total;
    volatile int num_chunks;
    volatile MMapResult *chunks;
  } mmap;

  struct {
    uint64_t flushed; // database size in number of pages
    uint64_t num_temp;
    char *temp; // newly allocated pages
  } page;
} KV;

extern KV global_kv;

void kvInit();
void kvOpen();
void kvClose();

Bytes kvGet(Bytes key);
void kvSet(Bytes key, Bytes val);
int kvDelete(Bytes key);

#endif