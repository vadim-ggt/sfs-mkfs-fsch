#ifndef OPERATIONS_H
#define OPERATIONS_H

#define FUSE_USE_VERSION 30

#include "filetype.h"
#include "inode.h"
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include "fuse.h"
#include "fs_init.h"
// Добавьте определения, если их нет. Подстройте block_size под вашу систему, если он отличается.
#ifndef block_size
#define block_size 4096 // Пример: 4KB блок. Убедитесь, что это соответствует вашей SuperBlock.
#endif
#define MAX_BLOCKS 16 // Максимальное количество блоков на один inode (у вас уже есть)
// Максимальный размер файла для inode с 16 прямыми блоками
// Убедитесь, что int в inode достаточно большой для этого значения
#define MAX_FILE_SIZE (MAX_BLOCKS * block_size)

# define UTIME_NOW	((1l << 30) - 1l)
# define UTIME_OMIT	((1l << 30) - 2l)

int sfs_mkdir(const char *path, mode_t mode);

int sfs_getattr(const char *path, struct stat *stat_buf);

int sfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);

int sfs_rmdir(const char *path);

int sfs_rm(const char *path);

int sfs_open(const char *path, struct fuse_file_info *fi);

int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int sfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int sfs_rename(const char *from, const char *to);

int sfs_utimens(const char *path, const struct timespec tv[2]);

int sfs_release(const char *path, struct fuse_file_info *fi);

void sfs_destroy(void *private_data);

int sfs_truncate(const char *path, off_t size);


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
extern struct fuse_operations operations;
#pragma GCC diagnostic pop

#endif 
