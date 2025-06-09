#ifndef UTILITIES_H
#define UTILITIES_H

#include "../include/filetype.h"

typedef struct filetype filetype;
typedef struct inode inode;

void deserialize_filetype_from_file(filetype *f, FILE *fp);
void serialize_filetype_to_file(filetype *f, FILE *fp);
void deserialize_inode_from_file(inode *i, FILE *fp);
void serialize_inode_to_file(inode *i, FILE *fp);
void serialize_superblock_to_file(superblock *sb, FILE *fp);
void deserialize_superblock_from_file(superblock *sb, FILE *fp);
char* get_file_name(const char* path);
char* get_file_path(const char* path);
void free_filetype(filetype *node);
#endif 
