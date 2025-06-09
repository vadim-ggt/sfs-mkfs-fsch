#ifndef FS_INIT_H
#define FS_INIT_H

#include "../include/inode.h"
#include "../include/superblock.h"
#include "../include/filetype.h"
#include "../include/operations.h"
#include "../include/utilities.h"

#define S_IFDIR 0x4000

void root_dir_init();

int save_system_state();

void restore_file_system();

#endif 