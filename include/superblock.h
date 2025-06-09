
#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

#include <string.h>

#define block_size 1024

typedef struct superblock {
    char data_blocks[block_size * 100];  // Total number of data blocks
    char data_bitmap[105];               // Array of available data block numbers
    char inode_bitmap[105];              // Array of available inode numbers
} superblock;

extern superblock s_block;

void superblock_init();

int find_free_db();

#endif 