#ifndef FILETYPE_H
#define FILETYPE_H

#include <../include/stdio.h>
#include <../include/string.h>
#include <../include/stdlib.h>
#include "../include/inode.h"
#include "../include/fs_init.h"

#define MAX_FILES 31

typedef struct inode inode;

typedef struct filetype {
    int valid;                   // Flag indicating if the filetype is valid
    char path[100];              // Path of the filetype
    char name[100];              // Name of the filetype
    inode *inum;                 // Pointer to the inode associated with the filetype
    struct filetype **children;  // Array of pointers to child filetypes
    int num_children;            // Number of child filetypes
    int num_links;               // Number of links to the filetype
    struct filetype *parent;     // Pointer to the parent filetype
    char type[20];               // Type of the filetype
} filetype;

extern char *strdup(const char *s);
extern filetype *root;
extern filetype file_array[MAX_FILES];

filetype *filetype_from_path(const char *path);

void remove_child(filetype *parent, filetype *child);


#endif
