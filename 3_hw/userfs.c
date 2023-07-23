#include "userfs.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "heap_help.h"
#include "utils/heap_help/heap_help.h"

enum {
  BLOCK_SIZE = 512,
  MAX_FILE_SIZE = 1024 * 1024 * 100,
};

/** Global error code. Set from any function on any error. */
static enum ufs_error_code ufs_error_code = UFS_ERR_NO_ERR;

struct block {
  /** Block memory. */
  char *memory;
  /** How many bytes are occupied. */
  int occupied;
  /** Next block in the file. */
  struct block *next;
  /** Previous block in the file. */
  struct block *prev;

  /* PUT HERE OTHER MEMBERS */
};

struct file {
  /** Double-linked list of file blocks. */
  struct block *block_list;
  /**
   * Last block in the list above for fast access to the end
   * of file.
   */
  struct block *last_block;
  /** How many file descriptors are opened on the file. */
  int refs;
  /** File name. */
  char *name;
  /** Files are stored in a double-linked list. */
  struct file *next;
  struct file *prev;

  int blockAmount;
  int is_deleted;

  /* PUT HERE OTHER MEMBERS */
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
  struct file *file;
  int fd;
  struct block *block; /* block to write in */
  int offset;
  int perm; /* permission flag */
  /* PUT HERE OTHER MEMBERS */
};

/**
 * An array of file descriptors. When a file descriptor is
 * created, its pointer drops here. When a file descriptor is
 * closed, its place in this array is set to NULL and can be
 * taken by next ufs_open() call.
 */
static struct filedesc **file_descriptors = NULL;
static int file_descriptor_count = 0;
static int file_descriptor_capacity = 0;

enum ufs_error_code ufs_errno() { return ufs_error_code; }

int ufs_open(const char *filename, int flag) {
  struct file *f = file_list;
  while (f != NULL && strcmp(f->name, filename)) f = f->prev;

  /* If a file does not exist, then create it (if UFS_CREATE is in place). */
  if (f == NULL) {
    if (flag == UFS_CREATE) {
      struct file *file = calloc(1, sizeof(struct file));
      file->name = strdup(filename);
      if (file_list != NULL) file_list->next = file;
      file->prev = (file_list == NULL) ? NULL : file_list;
      file->next = NULL;
      file->blockAmount = 0;
      file->is_deleted = 0;
      file_list = file;
      f = file_list;
    } else {
      ufs_error_code = UFS_ERR_NO_FILE;
      return -1;
    }
  }

  /* Create new fds if they run out */
  if (file_descriptor_count == file_descriptor_capacity) {
    file_descriptors =
        realloc(file_descriptors,
                sizeof(struct filedesc *) * (file_descriptor_count + 5));
    file_descriptor_capacity += 5;
    for (int i = file_descriptor_count; i < file_descriptor_capacity; i++) {
      file_descriptors[i] = NULL;
    }
  }

  /* Find a fd with the least possible id */
  int minFD;
  for (int i = 0; i < file_descriptor_capacity; i++) {
    if (!file_descriptors[i]) {
      minFD = i;
      break;
    }
  }

  /* Create fd and return id */
  struct filedesc *fdesc = calloc(1, sizeof(struct filedesc));
  fdesc->fd = minFD; /* pick the minimum available fd */
  fdesc->file = f;
  fdesc->block = f->block_list;
  fdesc->offset = 0;
  fdesc->perm = flag;
  file_descriptors[minFD] = fdesc;
  file_descriptor_count++;

  f->refs++;
  return minFD;
}

ssize_t ufs_write(int fd, const char *buf, size_t size) {
  int _size = size;

  if ((fd >= file_descriptor_capacity) || (fd < 0)) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  if (!file_descriptors[fd]) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  if (file_descriptors[fd]->perm == UFS_READ_ONLY) {
    ufs_error_code = UFS_ERR_NO_PERMISSION;
    return -1;
  }

  struct file *f = file_descriptors[fd]->file;
  if (f->blockAmount >= MAX_FILE_SIZE / BLOCK_SIZE) {
    ufs_error_code = UFS_ERR_NO_MEM;
    return -1;
  }

  /* put fd pointer at the blocks' start */
  if (file_descriptors[fd]->block == NULL) {
    file_descriptors[fd]->block = f->block_list;
  }

  /* if no blocks exist for this file */
  if (file_descriptors[fd]->block == NULL) {
    struct block *block = calloc(1, sizeof(struct block));
    block->memory = calloc(BLOCK_SIZE, 1);
    block->prev = NULL;
    block->next = NULL;
    block->occupied = 0;

    f->block_list = block;
    f->last_block = block;
    f->blockAmount += 1;

    file_descriptors[fd]->block = block;
    file_descriptors[fd]->offset = 0;
  }

  /* now, we can start to write buf in blocks */
  while (_size > 0) {
    int *occupied = &(file_descriptors[fd]->block->occupied);
    int *offset = &(file_descriptors[fd]->offset);
    int bytesToWrite = (_size > BLOCK_SIZE) ? BLOCK_SIZE : _size;

    if (bytesToWrite <= BLOCK_SIZE - *offset) {
      memmove(file_descriptors[fd]->block->memory + *offset, buf, bytesToWrite);
      buf += BLOCK_SIZE - *occupied;
      if (bytesToWrite >= *occupied - *offset)
        *occupied += bytesToWrite - (*occupied - *offset);
      *offset += bytesToWrite;
      _size -= bytesToWrite;
    } else {
      /* if the current block has not enough space then fill it till the end
      and create the next one */
      if (BLOCK_SIZE - *occupied != 0) {
        memmove(file_descriptors[fd]->block->memory + *occupied, buf,
                BLOCK_SIZE - *occupied);
        _size -= BLOCK_SIZE - *occupied;
        buf += BLOCK_SIZE - *occupied;

        *occupied = BLOCK_SIZE;
      }

      if (f->blockAmount >= MAX_FILE_SIZE / BLOCK_SIZE) {
        ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
      }

      struct block *newBlock = calloc(1, sizeof(struct block));
      newBlock->memory = calloc(BLOCK_SIZE, 1);
      newBlock->prev = file_descriptors[fd]->block;
      file_descriptors[fd]->block->next = newBlock;
      newBlock->next = NULL;
      newBlock->occupied = 0;

      f->last_block = newBlock;
      f->blockAmount += 1;

      file_descriptors[fd]->block = newBlock;
      file_descriptors[fd]->offset = 0;
    }
  }

  return size;
}

ssize_t ufs_read(int fd, char *buf, size_t size) {
  int _size = size;

  if ((fd >= file_descriptor_capacity) || (fd < 0)) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  if (!file_descriptors[fd]) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  if (file_descriptors[fd]->perm == UFS_WRITE_ONLY) {
    ufs_error_code = UFS_ERR_NO_PERMISSION;
    return -1;
  }

  struct file *f = file_descriptors[fd]->file;
  /* put fd pointer at the blocks' start if fd pointer is NULL */
  if (file_descriptors[fd]->block == NULL) {
    file_descriptors[fd]->block = f->block_list;
    file_descriptors[fd]->offset = 0;
  }

  /* block and offset where to read from. */
  struct block *block = file_descriptors[fd]->block;
  int *offset = &(file_descriptors[fd]->offset);
  if (!block) return 0;
  int k = 0;

  while ((block != NULL) && (_size > 0)) {
    int bytesToRead =
        (_size > block->occupied - *offset) ? block->occupied - *offset : _size;

    memmove(buf + k, block->memory + *offset,
            bytesToRead);  // buf can be smaller!!
    k += bytesToRead;

    *offset += bytesToRead;

    if (*offset == BLOCK_SIZE) {
      file_descriptors[fd]->block = block->next;
      *offset = 0;
    }

    block = block->next;
    _size -= bytesToRead;

    // if (_size < 0) _size = 0;
  }

  return size - _size;
}

int ufs_close(int fd) {
  if (fd < 0 || fd >= file_descriptor_capacity || !file_descriptors[fd]) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  struct file *f = file_descriptors[fd]->file;
  f->refs--;

  /* If no open descriptors left, the file contents are to delete. */
  if ((f->is_deleted) && (f->refs == 0)) {
    free(f->name);
    struct block *last = f->last_block;
    while (last) {
      struct block *p = last->prev;
      free(last->memory);
      free(last);
      last = p;
    }
    free(f);  // Free struct file itself.
  }

  file_descriptor_count--;
  free(file_descriptors[fd]);
  file_descriptors[fd] = NULL;
  return 0;
}

int ufs_delete(const char *filename) {
  struct file *f = file_list;
  while (f != NULL && strcmp(f->name, filename)) f = f->prev;
  if (!f) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  /* Switch is_deleted flag. */
  f->is_deleted = 1;

  /* Shift file_list pointer. */
  if (file_list == f) {
    file_list = f->prev;
  }

  /* Delete file from linked list of files. */
  if (f->next) f->next->prev = f->prev;
  if (f->prev) f->prev->next = f->next;

  return 0;
}

int ufs_resize(int fd, size_t new_size) {
  if ((fd >= file_descriptor_capacity) || (fd < 0)) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  if (!file_descriptors[fd]) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  if (new_size > MAX_FILE_SIZE) {
    ufs_error_code = UFS_ERR_NO_MEM;
    return -1;
  }

  struct file *f = file_descriptors[fd]->file;

  /* Count size of a file */
  size_t size = 0;
  struct block *p = f->last_block;
  while (p) {
    size += p->occupied;
    p = p->prev;
  }

  if (size <= new_size) {
    /* Increase size of a file*/
    size_t blocks_to_add = (new_size - size) % BLOCK_SIZE + 1;
    for (size_t i = 0; i < blocks_to_add; i++) {
      struct block *block = calloc(1, sizeof(struct block));
      block->memory = calloc(BLOCK_SIZE, 1);

      f->last_block->next = block;
      block->prev = f->last_block;
      f->last_block = block;
      f->blockAmount += 1;
    }
  } else {
    /* Shrink an offset of every file descriptor */
    for (int i = 0; i < file_descriptor_count; i++) {
      if (file_descriptors[i]->file == f) {
        int *offset = &(file_descriptors[i]->offset);
        int numBlocks = 0;
        struct block *last = file_descriptors[i]->block;
        while (last) {
          numBlocks++;
          last = last->prev;
        }

        if ((numBlocks - 1) * BLOCK_SIZE + *offset > (int)new_size) {
          int blockIndex = new_size / BLOCK_SIZE;
          struct block *block = f->block_list;
          for (int i = 0; i < blockIndex; i++) block = block->next;

          file_descriptors[i]->block = block;
          file_descriptors[i]->offset = new_size % BLOCK_SIZE;
        }
      }
    }

    /* Shrink a file */
    size_t blocks_to_free = f->blockAmount - (new_size / BLOCK_SIZE + 1);
    for (size_t i = 0; i < blocks_to_free; i++) {
      f->blockAmount--;
      struct block *last = f->last_block;

      if ((last) && (last->prev)) {
        last->prev->next = NULL;
      }

      if (f->last_block) {
        f->last_block = f->last_block->prev;
      }

      free(last->memory);
      free(last);
    }

    for (size_t i = new_size % BLOCK_SIZE; i < BLOCK_SIZE; i++) {
      f->last_block->memory[i] = '\0';
    }
    f->last_block->occupied = new_size % BLOCK_SIZE;

    // if (file_descriptors[fd]->perm == UFS_WRITE_ONLY) {
    //   ufs_error_code = UFS_ERR_NO_PERMISSION;
    //   return -1;
    // }
  }
  return 0;
}

void ufs_destroy(void) {
  /* free all file descriptors */
  for (int i = 0; i < file_descriptor_capacity; i++) {
    if (file_descriptors[i]) free(file_descriptors[i]);
  }
  free(file_descriptors);

  /* start to free files */
  struct file *last = file_list;
  while (last) {
    struct file *prev = last->prev;
    free(last->name);

    /* free all file blocks */
    struct block *lastBlock = last->last_block;
    while (lastBlock) {
      struct block *prev = lastBlock->prev;
      free(prev->memory);
      lastBlock = prev;
    }

    /* free a file struct */
    free(last);

    last = prev;
  }
}
