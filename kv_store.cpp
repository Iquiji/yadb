#include "btree_ext.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "stdio.h"

struct KV {
  char *path;

  FILE *fp;
  BTree *tree;

  struct {
    int file;
    int total;
    char chunks[0][0];
  } mmap;

  struct {
    u_int64_t flushed;
    char temp[0][0];
  } page;
};

struct MMapResult {
  int size;
  char *bytes;
};

MMapResult mmapInit(FILE *fp) {
  struct stat sb;
  stat((char *)fp, &sb);


  if (sb.st_size % BTREE_PAGE_SIZE != 0) {
    fprintf(stderr, "mmapInit: File not page aligned!");
    exit(1);
  }

  fp.fd;

  int mmapSize = 64 << 20;
  assert(mmapSize % BTREE_PAGE_SIZE == 0);
  while (mmapSize < sb.st_size) {
    mmapSize *= 2;
  }

  int fd = shm_open(fp, O_RDWR, S_IRUSR | S_IWUSR);
  void *addr = mmap(NULL, mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  return MMapResult{mmapSize, (char *)addr};
}

void extendMmap(KV *kv, int npages) {
  if (kv->mmap.total >= npages * BTREE_PAGE_SIZE)
    return;


}