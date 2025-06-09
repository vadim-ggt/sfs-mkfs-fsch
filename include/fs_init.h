#ifndef FS_INIT_H
#define FS_INIT_H

#include "../include/inode.h"
#include "../include/superblock.h"
#include "../include/filetype.h"
#include "../include/utilities.h"
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define S_IFDIR 0x4000
#define PATH_MAX 200


extern char file_struct_path_global[256];
extern char super_path_global[256];

void root_dir_init();

int save_system_state();

void restore_file_system(const char *super_path, const char *file_struct_path);

void set_fs_paths(const char *super_path, const char *file_struct_path);

bool ask_for_format_confirmation();

void cleanup_filesystem();

#endif 