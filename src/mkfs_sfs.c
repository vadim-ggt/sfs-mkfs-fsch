
#include "../include/fs_init.h"
#include <stdio.h>

int main(int argc, char *argv[]){

    if (argc < 2) {
        printf("Usage: %s <sfs_directory>\n", argv[0]);
        return 1;
    }
    const char *sfs_path = argv[1];
    char super_path[256];
    char file_struct_path[256];
    snprintf(super_path, sizeof(super_path), "%s/super.bin", sfs_path);
    snprintf(file_struct_path, sizeof(file_struct_path), "%s/file_structure.bin", sfs_path);
    restore_file_system(super_path, file_struct_path );
    cleanup_filesystem();
    return 0;
}