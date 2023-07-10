#include "userfs.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "heap_help.h"

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

  /* PUT HERE OTHER MEMBERS */
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
  struct file *file;
  int fd;
  struct block *block; /* block to write in */
  int offset;
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

int ufs_open(const char *filename, int flags) {
  struct file *f = file_list;
  while (f != NULL && !strcmp(f->name, filename)) f = f->prev;

  /* If a file does not exist, then create it (if UFS_CREATE is in place). */
  if (f == NULL) {
    if (UFS_CREATE) {
      struct file *file = malloc(sizeof(struct file));
      file->name = strdup(filename);
      if (file_list != NULL) file_list->next = file;
      file->prev = (file_list == NULL) ? NULL : file_list;
      file->next = NULL;
      file->refs++;
      file->blockAmount = 0;
      file_list = file;

      if (file_descriptor_capacity == 0) {
        file_descriptors = calloc(5, sizeof(struct filedesc *));
      } else if (file_descriptor_count == file_descriptor_capacity) {
        file_descriptors = realloc(file_descriptors, file_descriptor_count + 5);
      }

      struct filedesc *fdesc = calloc(1, sizeof(struct filedesc));
      fdesc->fd = file_descriptor_count;
      fdesc->file = file;
      fdesc->offset = 0;
      file_descriptors[file_descriptor_count++] = fdesc;

      return file_descriptor_count++;
    } else {
      ufs_error_code = UFS_ERR_NO_FILE;
      return -1;
    }
  } else {
    /* If the file really exists, then create new struct filedesc. */
    if (file_descriptor_capacity == 0) {
      file_descriptors = calloc(5, sizeof(struct filedesc *));
    } else if (file_descriptor_count == file_descriptor_capacity) {
      file_descriptors = realloc(file_descriptors, file_descriptor_count + 5);
    }

    struct filedesc *fdesc = calloc(1, sizeof(struct filedesc));
    fdesc->fd = file_descriptor_count;
    fdesc->file = f;
    fdesc->block = NULL;
    file_descriptors[file_descriptor_count++] = fdesc;

    return file_descriptor_count++;
  }
}

ssize_t ufs_write(int fd, const char *buf, size_t size) {
  int _size = size;
  if ((fd < file_descriptor_capacity) || (!(file_descriptors + fd))) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  struct file *f = file_descriptors[fd]->file;
  struct block *block = file_descriptors[fd]->block;

  /* if no blocks exist for this file */
  if (block == NULL) {
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
    int *occupied = &file_descriptors[fd]->block->occupied;
    int bytesToWrite = (_size > BLOCK_SIZE) ? BLOCK_SIZE : _size;

    if (bytesToWrite <= BLOCK_SIZE - *occupied) {
      memmove(file_descriptors[fd]->block + *occupied, buf, bytesToWrite);
      *occupied += bytesToWrite;
      _size -= bytesToWrite;
    } else {
      /* if the current block has no enough space then fill it till the end
      and create the next one */
      if (BLOCK_SIZE - *occupied != 0) {
        memmove(file_descriptors[fd]->block + *occupied, buf,
                BLOCK_SIZE - *occupied);
        *occupied = BLOCK_SIZE;
        _size -= BLOCK_SIZE - *occupied;
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
  if ((fd < file_descriptor_capacity) || (!(file_descriptors + fd))) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  struct file *f = file_descriptors[fd]->file;
  struct block *block = f->block_list;
  if (!block) return 0;

  while ((block != NULL) && (_size > 0)) {
    int bytesToWrite = (_size > block->occupied) ? block->occupied : _size;

    char *mem = calloc(bytesToWrite, 1);
    memmove(mem, block->memory, bytesToWrite);
    strcat(buf, mem);  // no null char?
    free(mem);

    block = block->next;
    _size -= bytesToWrite;
  }

  return size;
}

int ufs_close(int fd) {
  if ((fd < file_descriptor_capacity) || (!(file_descriptors + fd))) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  if (file_descriptors[fd]->file->refs-- == 0) {
    /* The file does not have opened file descriptors. */
  }
  file_descriptor_count--;
  free(file_descriptors[fd]);
}

int ufs_delete(const char *filename) {
  // Если на файл есть открытые дескрипторы, то файл должен стать недоступен для
  // следующих открытий. И должен удалиться, когда будет закрыт его последний
  // дескриптор
  struct file *f = file_list;
  while (f != NULL && !strcmp(f->name, filename)) f = f->prev;
  if (!f) {
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
  }

  /* Delete file from linked list of files */
  file_list = (f->prev) ? f->prev : NULL;
  if (!file_list) file_list = NULL;
  f->next->prev = (f->next) ? f->prev : NULL;
  f->prev->next = (f->prev) ? f->next : NULL;

  if (f->refs == 0) {
    /* If no open descriptors left, the file contents are to delete. */
    free(f->name);
    struct block *last = f->last_block;
    while (last) {
      struct block *p = last->prev;
      free(last->memory);
      free(last);
      last = p;
    }
  }
}

void ufs_destroy(void) {}
