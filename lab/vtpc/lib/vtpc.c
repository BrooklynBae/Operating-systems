#define _GNU_SOURCE
#include "vtpc.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define ALIGNMENT 4096
#define ARR_SIZE 256

typedef struct {
  off_t page_offset;
  void *block_bytes;
  int is_dirty;
  int valid;
  int ref;
} Page;

static Page cache[ARR_SIZE];
static int are_inited = 0;
static int hand = 0;
static off_t logical_size = 0;

void init_pages() {
  if (are_inited) {
    return;
  }
  int idx = 0;
  for (idx = 0; idx < ARR_SIZE; idx++) {
    cache[idx].page_offset = -1;
    cache[idx].is_dirty = 0;
    cache[idx].valid = 0;
    cache[idx].ref = 0;
    cache[idx].block_bytes = NULL;

    if (posix_memalign(&cache[idx].block_bytes, ALIGNMENT, PAGE_SIZE) != 0) {
      perror("posix_memalign");
      _exit(1);
    }
  }

  are_inited = 1;
}

static void reset_cache_metadata(void) {
  int idx = 0;
  for (idx = 0; idx < ARR_SIZE; idx++) {
    cache[idx].page_offset = -1;
    cache[idx].is_dirty = 0;
    cache[idx].valid = 0;
    cache[idx].ref = 0;
  }
  hand = 0;
}

static void refresh_logical_size(int fd) {
  struct stat st;
  if (fstat(fd, &st) == 0) {
    logical_size = st.st_size;
  }
}

static int find_page(off_t page_off) {
  int idx = 0;
  for (idx = 0; idx < ARR_SIZE; idx++) {
    if (cache[idx].valid && cache[idx].page_offset == page_off) {
      return idx;
    }
  }
  return -1;
}

static int flush_page(int fd, int idx) {
  if (!cache[idx].valid || !cache[idx].is_dirty) {
    return 0;
  }
  ssize_t w = pwrite(fd, cache[idx].block_bytes, PAGE_SIZE, cache[idx].page_offset);
  if (w < 0) {
    return -1;
  }
  if (w != PAGE_SIZE) { 
    errno = EIO; 
    return -1; 
  }

  cache[idx].is_dirty = 0;
  return 0;
}

static int load_page_from_disk(int fd, int idx, off_t page_off) {
  ssize_t n = pread(fd, cache[idx].block_bytes, PAGE_SIZE, page_off);
  if (n < 0) {
    return -1;
  }

  if (n == 0) {
    memset(cache[idx].block_bytes, 0, PAGE_SIZE);
  } else if (n < PAGE_SIZE) {
    memset((char*)cache[idx].block_bytes + n, 0, PAGE_SIZE - n);
  }

  cache[idx].page_offset = page_off;
  cache[idx].valid = 1;
  cache[idx].ref = 1;
  cache[idx].is_dirty = 0;
  return 0;
}

static int alloc_slot_clock(int fd) {
  int idx = 0;
  for (idx = 0; idx < ARR_SIZE; idx++) {
    if (!cache[idx].valid) {
        return idx;
    }
  }

  while (1) {
    if (cache[hand].ref) {
      cache[hand].ref = 0;
      hand = (hand + 1) % ARR_SIZE;
      continue;
    }

    if (flush_page(fd, hand) < 0) {
      return -1;
    }

    int victim = hand;
    hand = (hand + 1) % ARR_SIZE;
    return victim;
  }
}

int vtpc_open(const char* path, int mode, int access) {
  init_pages();

  int fd = open(path, mode, access);
  if (fd < 0) {
    return fd;
  }

  if ((unsigned)mode & O_TRUNC) {
    reset_cache_metadata();
    logical_size = 0;
  } else {
    refresh_logical_size(fd);
  }

  return fd;
}

int vtpc_close(int fd) {
  (void)vtpc_fsync(fd);
  return close(fd);
}

off_t vtpc_lseek(int fd, off_t offset, int whence) {
  return lseek(fd, offset, whence);
}

ssize_t vtpc_write(int fd, const void* buf, size_t count) {

  const char *in = (const char*)buf;
  size_t left = count;
  ssize_t total = 0;

  while (left > 0) {
    off_t cur = vtpc_lseek(fd, 0, SEEK_CUR);

    off_t page_off = (cur / PAGE_SIZE) * PAGE_SIZE;
    size_t in_page = (size_t)(cur - page_off);

    size_t chunk = left;
    if (in_page + chunk > PAGE_SIZE) {
      chunk = PAGE_SIZE - in_page;
    }

    int idx = find_page(page_off);
    if (idx < 0) {
      idx = alloc_slot_clock(fd);
      if (idx < 0) {
        if (total > 0) {
            return total;
        }
        return -1;
      }
      if (load_page_from_disk(fd, idx, page_off) < 0) {
        if (total > 0) {
            return total;
        }
        return -1;
      }
    }

    memcpy((char*)cache[idx].block_bytes + in_page, in, chunk);
    cache[idx].is_dirty = 1;
    cache[idx].ref = 1;

    off_t new_end = cur + (off_t)chunk;
    if (new_end > logical_size) {
        logical_size = new_end;
    }

    vtpc_lseek(fd, (off_t)chunk, SEEK_CUR);
    in += chunk;
    left -= chunk;
    total += (ssize_t)chunk;
  }

  return total;
}

ssize_t vtpc_read(int fd, void* buf, size_t count) {

  char *out = (char*)buf;
  size_t left = count;
  ssize_t total = 0;

  while (left > 0) {
    off_t cur = vtpc_lseek(fd, 0, SEEK_CUR);

    if (cur >= logical_size) {
        return total;
    }

    off_t page_off = (cur / PAGE_SIZE) * PAGE_SIZE;
    size_t in_page = (size_t)(cur - page_off);

    size_t chunk = left;
    if (in_page + chunk > PAGE_SIZE) {
        chunk = PAGE_SIZE - in_page;
    }

    off_t remaining = logical_size - cur;
    if ((off_t)chunk > remaining) {
        chunk = (size_t)remaining;
    }
    if (chunk == 0) {
        return total;
    }

    int idx = find_page(page_off);
    if (idx < 0) {
      idx = alloc_slot_clock(fd);
      if (idx < 0) {
        if (total > 0) {
            return total;
        }
        return -1;
      }
      int load_res = load_page_from_disk(fd, idx, page_off);
      if (load_res < 0) {
        if (total > 0) {
            return total;
        }
        return -1;
      }
    }

    memcpy(out, (char*)cache[idx].block_bytes + in_page, chunk);
    cache[idx].ref = 1;

    vtpc_lseek(fd, (off_t)chunk, SEEK_CUR);
    out += chunk;
    left -= chunk;
    total += (ssize_t)chunk;
  }

  return total;
}

int vtpc_fsync(int fd) {
  init_pages();
  int idx = 0;

  for (idx = 0; idx < ARR_SIZE; idx++) {
    if (cache[idx].valid && cache[idx].is_dirty) {
      if (flush_page(fd, idx) < 0) {
        return -1;
      }
    }
  }

  if (ftruncate(fd, logical_size) < 0) {
    return -1;
  }

  return fsync(fd);
}
