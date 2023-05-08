#include "kv_store.h"

MMapResult mmapInit() {
  struct stat sb;

  int fstat_res = fstat(global_kv.fd, &sb);
  if (fstat_res) {
    printf("Bad fstat call: reason: %s\n", strerror(errno));
  }
  printf("file descriptor: %d\n", global_kv);
  printf("size: %lu\n", sb.st_size);
  if (sb.st_size % BTREE_PAGE_SIZE != 0) {
    fprintf(stderr, "mmapInit: File not page aligned!");
    exit(1);
  }

  uint64_t mmapSize = 64 << 20;
  assert(mmapSize % BTREE_PAGE_SIZE == 0);
  while (mmapSize < sb.st_size) {
    mmapSize *= 2;
  }

  void *addr = mmap(NULL, mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, global_kv.fd, 0);
  if (addr == MAP_FAILED) {
    fprintf(stderr, "mmap size_of_segment=%ld fd=%d failed %s\n", mmapSize, global_kv.fd, strerror(errno));
    exit(EXIT_FAILURE);
  };

  printf("+ mmap address: %lx\n", addr);

  MMapResult result = {mmapSize, (char *)addr};
  printf("MMapResult.size: %lu\n", result.size);
  return result;
}

char *appendToMMapResultList(MMapResult *list, int size, MMapResult new_element) {
  MMapResult *new_list = (MMapResult *)malloc((size + 1) * 16);
  memcpy(new_list, list, size * 16);
  new_list[size] = new_element;
  return new_list;
}

void extendMmap(KV *kv, int npages) {
  if (kv->mmap.total >= npages * BTREE_PAGE_SIZE)
    return;

  // double address space by making a new map
  void *addr = mmap(NULL, kv->mmap.total, PROT_READ | PROT_WRITE, MAP_SHARED, kv->fd, kv->mmap.total);

  MMapResult result = {kv->mmap.total, (char *)addr};

  kv->mmap.total *= 2;
  void *old_chunks = kv->mmap.chunks;
  kv->mmap.chunks = appendToMMapResultList(kv->mmap.chunks, kv->mmap.num_chunks, result);
  // free(old_chunks);
  kv->mmap.num_chunks += 1;
}

// Assume global kv
KV global_kv;
const char *DB_SIG = "MyKVDB.........";

BTreeNode *page_get_callback(uint64_t page_ptr) {
  uint64_t start = 0;

  for (int i = 0; i < global_kv.mmap.num_chunks; ++i) {
    uint64_t end = start + global_kv.mmap.chunks[i].size / BTREE_PAGE_SIZE;
    // if (global_kv.mmap.num_chunks > 1) {
    //   printf("global_kv.mmap.num_chunks > 1: %d\n", global_kv.mmap.num_chunks);
    //   printf("page_ptr: %lu", page_ptr);
    //   printf("start: %lu, end: %lu\n", start);
    // }

    if (page_ptr < end) {
      uint64_t offset = BTREE_PAGE_SIZE * (page_ptr - start);
      // printf("MmapResult: size: %lu\n", global_kv.mmap.chunks[i].size);
      // printf(" +pointer: %lx\n", global_kv.mmap.chunks[i].bytes);
      BTreeNode *res = (BTreeNode *)(global_kv.mmap.chunks[i].bytes + offset);
      // BTreeNodeInfo(res);
      return res;
    }
    start = end;
  }

  fprintf(stderr, "BAD Page Pointer: %lx", page_ptr);
  exit(1);
}

// the master page format.
// it contains the pointer to the root and other important bits.
// | sig | btree_root | page_used |
// | 16B | 8B | 8B |
void masterPageLoad() {
  if (global_kv.mmap.file == 0) {
    global_kv.page.flushed = 1;
    return;
  }

  MMapResult data = global_kv.mmap.chunks[0];
  uint64_t root = *(uint64_t *)(data.bytes + 16);
  uint64_t used = *(uint64_t *)(data.bytes + 24);

  if (memcmp(DB_SIG, (char *)global_kv.mmap.chunks, 16) != 0) {
    fprintf(stderr, "BAD Page Signature!");
    exit(1);
  }

  uint64_t bad = !(1 <= used && used <= global_kv.mmap.file / BTREE_PAGE_SIZE);
  bad = bad || !(0 <= root && root < used);
  if (bad) {
    fprintf(stderr, "BAD Master Page!");
    exit(1);
  }

  global_kv.tree->root = root;
  global_kv.page.flushed = used;
}

void masterPageStore() {
  char data[32];
  memcpy(data, DB_SIG, 16);
  *(uint64_t *)(data + 16) = (uint64_t)(global_kv.tree->root);
  *(uint64_t *)(data + 24) = (uint64_t)global_kv.page.flushed;

  // NOTE: Updating the page via mmap is not atomic.
  // Use the `pwrite()` syscall instead.
  int res = pwrite(global_kv.fd, data, 32, 0);
}

char *appendToBTreeNodeList(char *list, int size, BTreeNode *new_element) {
  BTreeNode **new_list = (BTreeNode **)malloc((size + 1) * 8);
  memcpy(new_list, list, size * 8);
  new_list[size] = new_element;
  // printf("new_list: %lx\n", new_list);
  // printf("size = %d\n", size);
  // if (size > 0) printf("list[size - 1] = %lx\n", ((BTreeNode **)list)[size - 1]);
  // printf("new_list[size] = %lx\n", new_list[size]);
  // if (size > 0) printf("new_list[size - 1] = %lx\n",new_list [size - 1]);
  // new_list[size] = new_element;

  return new_list;
}

uint64_t page_new_callback(BTreeNode *node_ptr) {
  assert(BTNodeSize(node_ptr) <= BTREE_PAGE_SIZE);
  uint64_t ptr = global_kv.page.flushed + global_kv.page.num_temp;

  // printf("page_new_callback: %lx -> %lu\n",node_ptr,ptr);

  void *old_temp = global_kv.page.temp;
  char* new_page_temp = appendToBTreeNodeList(global_kv.page.temp, global_kv.page.num_temp, node_ptr);
  // printf("new_page_temp: %lx\n", new_page_temp);
  // printf("new_page_temp[0]: %lx\n", ((uint64_t*)new_page_temp)[0]);
  global_kv.page.temp = new_page_temp;
  // TODO:
  // printf("Want to free %lx\n", old_temp);
  // free(old_temp);
  global_kv.page.num_temp += 1;

  // printf("global_kv.page.temp: %lx: \n", global_kv.page.temp);
  // for (int i = 0; i < global_kv.page.num_temp; ++i){
  //   printf("  + %lx\n", ((uint64_t**)global_kv.page.temp)[i]);
  // }

  return ptr;
}

void page_delete_callback(uint64_t ptr) {
  // TODO:
}

void extendFile(int npages) {
  uint64_t file_pages = global_kv.mmap.file / BTREE_PAGE_SIZE;
  if (file_pages >= npages) {
    return;
  }

  while (file_pages < npages) {
    int inc = file_pages / 8;
    if (inc < 1) {
      inc = 1;
    }
    file_pages += inc;
  }

  uint64_t file_size = file_pages * BTREE_PAGE_SIZE;
  fallocate(global_kv.fd, 0, 0, file_size);

  global_kv.mmap.file = file_size;
}

void kvInit() {
  memset(&global_kv, 0, sizeof(KV));
  global_kv.tree = malloc(sizeof(BTree));
  memset(global_kv.tree, 0, sizeof(BTree));
}

void kvOpen() {
  global_kv.fd = open(global_kv.path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (global_kv.fd < 0){
    fprintf(stderr, "file descriptor < 0: %d, reason: %s\n",global_kv.fd, strerror(errno));
    printf("address markes as bad: '%s'\n", global_kv.path);
    exit(1);
  }

  printf("file descriptor: %d\n", global_kv);

  MMapResult mmap_res = mmapInit();

  // hmm?
  printf("mmap_res.size: %lu\n", mmap_res.size);
  struct stat sb;
  fstat(global_kv.fd, &sb);

  global_kv.mmap.file = sb.st_size;
  global_kv.mmap.total = mmap_res.size;

  void *chunk_list_malloc = malloc(16);
  memcpy(chunk_list_malloc, &mmap_res, 16);
  global_kv.mmap.num_chunks = 1;
  global_kv.mmap.chunks = chunk_list_malloc;

  global_kv.tree->get_node = page_get_callback;
  global_kv.tree->new_page = page_new_callback;
  global_kv.tree->dealloc_page = page_delete_callback;

  printf("global_kv.mmap.file: %x\n", global_kv.mmap.file);
  masterPageLoad();
}

void kvFlushPages();

void kvClose() {
  for (int i = 0; i < global_kv.mmap.num_chunks; ++i) {
    munmap(global_kv.mmap.chunks[i].bytes, global_kv.mmap.chunks[i].size);
  }
}

Bytes kvGet(Bytes key) {
  return BTFind(global_kv.tree, key);
}

void kvSet(Bytes key, Bytes val) {
  BTInsert(global_kv.tree, key, val);
  kvFlushPages();
}

int kvDelete(Bytes key) {
  int deleted_flag = BTDelete(global_kv.tree, key);
  kvFlushPages();
  return deleted_flag;
}

void kvWritePages();
void kvSyncPages();

void kvFlushPages() {
  // printf("++ Flushing pages!\n");
  kvWritePages();
  kvSyncPages();
}

void kvWritePages() {
  uint64_t npages = global_kv.page.flushed + global_kv.page.num_temp;
  extendFile(npages);
  extendMmap(&global_kv, npages);

  for (int i = 0; i < global_kv.page.num_temp; ++i) {
    uint64_t ptr = global_kv.page.flushed + i;

    // printf("page_get_callback(ptr): %lx\n", page_get_callback(ptr));
    // BTreeNodeInfo((void*)(((uint64_t*)global_kv.page.temp)[i]));

    memcpy(page_get_callback(ptr), (void*)(((uint64_t*)global_kv.page.temp)[i]), BTREE_PAGE_SIZE);
  }
}

void kvSyncPages() {
  fsync(global_kv.fd);
  global_kv.page.flushed += global_kv.page.num_temp;
  free(global_kv.page.temp);
  global_kv.page.num_temp = 0;

  masterPageStore();
  fsync(global_kv.fd);
}