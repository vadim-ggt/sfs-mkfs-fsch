#ifndef INODE_H
#define INODE_H

#include "superblock.h"
#include "filetype.h"
#include "sys/types.h"

typedef struct filetype filetype;
typedef struct inode {
    int datablocks[16];        // Numbers of data blocks
    int number;                // Inode number
    int blocks;                // Number of data blocks
    int size;                  // Size of the file/directory
    mode_t permissions;        // Access permissions
    uid_t user_id;             // User identifier
    gid_t group_id;            // Group identifier
    time_t a_time;             // Access time
    time_t m_time;             // Modification time
    time_t c_time;             // Change time
    time_t b_time;             // Creation time
} inode;

int find_free_inode();

void add_child(filetype *parent, filetype *child);

#endif