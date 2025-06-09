#include "../include/fs_init.h"

filetype *root;


void print_superblock_details() {
    printf("Data blocks:\n");
    for (size_t i = 0; i < sizeof(s_block.data_blocks); i++) {
        printf("%c.", s_block.data_blocks[i]);
    }
    printf("\n");

    printf("Data Bitmap:\n");
    for (size_t i = 0; i < sizeof(s_block.data_bitmap); i++) {
        printf("%c.", s_block.data_bitmap[i]);
    }
    printf("\n");

    printf("Inode Bitmap:\n");
    for (size_t i = 0; i < sizeof(s_block.inode_bitmap); i++) {
        printf("%c.", s_block.inode_bitmap[i]);
    }
    printf("\n");
}


int save_system_state() {
    FILE *fd = fopen("file_structure.bin", "wb");
    if (!fd) {
        perror("Failed to open file_structure.bin for writing");
        return -1;
    }

    serialize_filetype_to_file(root, fd);

    FILE *fd1 = fopen("super.bin", "wb");
    if (!fd1) {
        perror("Failed to open super.bin for writing");
        fclose(fd);
        return -1;
    }

    serialize_superblock_to_file(&s_block, fd1);

    fclose(fd);
    fclose(fd1);

    return 0;
}


void restore_file_system() {
    FILE *fd = fopen("file_structure.bin", "rb");
    FILE *fd1 = fopen("super.bin", "rb");

    if (fd && fd1) {
        printf("File system restored!\n");

        root = calloc(1, sizeof(filetype));
        deserialize_filetype_from_file(root, fd);
        deserialize_superblock_from_file(&s_block, fd1);
        fclose(fd);
        fclose(fd1);
    } else {
        printf("SFS image not found! Please format the disk using mkfs.sfs\n");

        if (fd) fclose(fd);
        if (fd1) fclose(fd1);

        exit(1);  // Завершаем работу, чтобы не запускать fuse_main без ФС
    }
}
