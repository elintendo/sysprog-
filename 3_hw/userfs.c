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

  /* PUT HERE OTHER MEMBERS */
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
  struct file *file;
  int fd;
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
  // if(flags == UFS_CREATE) {}

  struct file *f = file_list;
  while (f != NULL && !strcmp(f->name, filename)) f = f->prev;

  /* If a file does not exist, then create it (if UFS_CREATE is in place). */
  if (f == NULL) {
    if (UFS_CREATE) {
      struct file *file = malloc(sizeof(struct file));
      file->name = strdup(filename);
      file->prev = NULL;
      file->next = NULL;
      file->refs++;
      file_list = file;

      if (file_descriptor_capacity == 0) {
        file_descriptors = calloc(5, sizeof(struct filedesc *));
      } else if (file_descriptor_count == file_descriptor_capacity) {
        file_descriptors = realloc(file_descriptors, file_descriptor_count + 5);
      }

      struct filedesc *fdesc = calloc(1, sizeof(struct filedesc));
      fdesc->fd = file_descriptor_count;
      fdesc->file = file;
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
    file_descriptors[file_descriptor_count++] = fdesc;

    return file_descriptor_count++;
  }

  // if (file_descriptors == NULL) {
  //   struct block *block = malloc(sizeof(struct block));
  //   block->memory = malloc(BLOCK_SIZE);
  //   block->prev = NULL;
  //   block->next = NULL;
  //   block->occupied = 0;
  //   file->block_list = block;

  //   file->last_block = block;
  // }

  /* IMPLEMENT THIS FUNCTION */
  // (void)filename;
  // (void)flags;
  // (void)file_list;
  // (void)file_descriptors;
  // (void)file_descriptor_count;
  // (void)file_descriptor_capacity;
  // ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
  // return -1;
}

ssize_t ufs_write(int fd, const char *buf, size_t size) {
  /* IMPLEMENT THIS FUNCTION */
  (void)fd;
  (void)buf;
  (void)size;
  ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
  return -1;
}

ssize_t ufs_read(int fd, char *buf, size_t size) {
  /* IMPLEMENT THIS FUNCTION */
  (void)fd;
  (void)buf;
  (void)size;
  ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
  return -1;
}

int ufs_close(int fd) {
  /* IMPLEMENT THIS FUNCTION */
  (void)fd;
  ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
  return -1;
}

int ufs_delete(const char *filename) {
  /* IMPLEMENT THIS FUNCTION */
  (void)filename;
  ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
  return -1;
}

void ufs_destroy(void) {}
