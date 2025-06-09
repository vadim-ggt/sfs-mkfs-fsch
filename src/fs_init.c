#include "../include/fs_init.h"
#include <unistd.h>

filetype *root;
 char file_struct_path_global[256];
 char super_path_global[256];

void root_dir_init() {
    s_block.inode_bitmap[1] = 1; 

    root = malloc(sizeof(filetype));
    memset(root, 0, sizeof(filetype));
    if (!root) {
        perror("Failed to allocate memory for root");
        return; 
    }

    strcpy(root->path, "/");
    strcpy(root->name, "/");
    strcpy(root->type, "directory");

    root->inum = malloc(sizeof(inode));
    memset(root->inum, 0, sizeof(inode));
    if (!root->inum) {
        perror("Failed to allocate memory for inode");
        free(root);
        return; 
    }

    root->inum->permissions = 0777;  
    root->inum->permissions |= S_IFDIR;
    time_t currentTime = time(NULL);
    root->inum->c_time = currentTime;
    root->inum->a_time = currentTime;
    root->inum->m_time = currentTime;
    root->inum->b_time = currentTime;
    root->inum->group_id = getgid();
    root->inum->user_id = getuid();
    root->children = NULL;
    root->parent = NULL;
    root->num_children = 0;
    root->num_links = 2;
    root->valid = 1;
    root->inum->size = 0;

    int index = find_free_inode();
    if (index == -1) {
        perror("Failed to find a free inode");
        cleanup_filesystem();
        free(root->inum);
        free(root);
        return;
    }
    root->inum->number = index;
    root->inum->blocks = 0;

    save_system_state();
}



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
    FILE *fd = fopen(file_struct_path_global, "wb");
    if (!fd) {
        perror("Failed to open file_structure for writing");
        return -1;
    }

    serialize_filetype_to_file(root, fd);
    fclose(fd);

    FILE *fd1 = fopen(super_path_global, "wb");
    if (!fd1) {
        perror("Failed to open superblock for writing");
        return -1;
    }

    serialize_superblock_to_file(&s_block, fd1);
    fclose(fd1);

    return 0;
}


bool ask_for_format_confirmation() {
    printf("Filesystem already exists. Format it? (y/N): ");
    char answer = getchar();
    while (getchar() != '\n'); 
    return (answer == 'y' || answer == 'Y');
}


void set_fs_paths(const char *super_path, const char *file_struct_path) {
    strncpy(super_path_global, super_path, sizeof(super_path_global) - 1);
    super_path_global[sizeof(super_path_global) - 1] = '\0';

    strncpy(file_struct_path_global, file_struct_path, sizeof(file_struct_path_global) - 1);
    file_struct_path_global[sizeof(file_struct_path_global) - 1] = '\0';
}



void restore_file_system(const char *super_path, const char *file_struct_path) {
    bool fs_exists = (access(super_path, F_OK) == 0) &&
                     (access(file_struct_path, F_OK) == 0);

    if (fs_exists) {
        if (ask_for_format_confirmation()) {
            printf("Formatting filesystem...\n");
            system("fusermount -u ~/mnt >/dev/null 2>&1");
            set_fs_paths(super_path, file_struct_path); 
            superblock_init();
            root_dir_init();
            save_system_state();

            printf("Filesystem formatted successfully.\n");

        } else {
            printf("Loading existing filesystem...\n");

            FILE* fd = fopen(file_struct_path, "rb");
            if (!fd) {
                perror("Failed to open file_structure");
                exit(EXIT_FAILURE);
            }

            root = malloc(sizeof(filetype));
            if (!root) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }
            memset(root, 0, sizeof(filetype));
            deserialize_filetype_from_file(root, fd);
            fclose(fd);

            FILE* fd1 = fopen(super_path, "rb");
            if (!fd1) {
                perror("Failed to open superblock");
                exit(EXIT_FAILURE);
            }

            deserialize_superblock_from_file(&s_block, fd1);
            fclose(fd1);

            set_fs_paths(super_path, file_struct_path); 
            

            printf("Filesystem loaded successfully.\n");
        }
    } else {
        printf("Creating new filesystem...\n");
        set_fs_paths(super_path, file_struct_path);  
        superblock_init();
        root_dir_init();
        save_system_state();
        printf("Filesystem created successfully.\n");
    }
}

void free_filetype(filetype *node) {
    if (!node) return;

    for (int i = 0; i < node->num_children; i++) {
        free_filetype(node->children[i]);  
    }

    free(node->inum);        
    free(node->children);    
    free(node);             
}

void cleanup_filesystem() {
    free_filetype(root);
    root = NULL;
}
