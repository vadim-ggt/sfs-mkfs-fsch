#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h> 
#include <stdarg.h>  
#include "../include/superblock.h"
#include "../include/filetype.h"

extern superblock s_block;
extern filetype *root;
bool debug_mode = false;

void print_debug(const char* format, ...);
bool load_superblock(const char *super_path);
filetype* deserialize_filetype(FILE *fp, filetype *parent); 
bool load_file_structure(const char *file_struct_path);
bool check_superblock_integrity();
bool check_filetype_node(filetype *node, int depth);
bool check_file_structure_integrity();
bool check_inode_integrity(inode *node);
bool check_all_inodes(filetype *node);
bool check_inode_integrity_in_filesystem();
void check_filesystem();


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <sfs_directory> [-d|--debug]\n", argv[0]);
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
            printf("=== DEBUG MODE ENABLED ===\n");
        }
    }

    const char *sfs_path = argv[1];

    char super_path[256];
    char file_struct_path[256];

    snprintf(super_path, sizeof(super_path), "%s/super.bin", sfs_path);
    snprintf(file_struct_path, sizeof(file_struct_path), "%s/file_structure.bin", sfs_path);

    if (!load_superblock(super_path)) {
        return 1;
    }
    
    if (!load_file_structure(file_struct_path)) {
        printf("Failed to load file structure.\n");
        return 1;
    }

    check_filesystem();

    cleanup_filesystem();

    return 0;
}










void print_debug(const char* format, ...) {
    if (debug_mode) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}



bool load_superblock(const char *super_path) {
    FILE *fp = fopen(super_path, "rb");
    if (!fp) {
        perror("Failed to open superblock file");
        return false;
    }

    size_t read_bytes = fread(&s_block, sizeof(superblock), 1, fp);
    fclose(fp);

    if (read_bytes != 1) {
        fprintf(stderr, "Failed to read superblock from file.\n");
        return false;
    }

    return true;
}



filetype* deserialize_filetype(FILE *fp, filetype *parent) {
    filetype *f = malloc(sizeof(filetype));
    if (!f) {
        fprintf(stderr, "Failed to allocate memory for filetype\n");
        return NULL;
    }

    fread(&f->valid, sizeof(int), 1, fp);
    fread(f->path, sizeof(char), 100, fp);
    fread(f->name, sizeof(char), 100, fp);
    
    int null_flag;
    fread(&null_flag, sizeof(int), 1, fp);
    if (null_flag == 1) {
        f->inum = malloc(sizeof(inode));
        if (!f->inum) {
            fprintf(stderr, "Failed to allocate memory for inode\n");
            free(f); 
            return NULL;
        }
        deserialize_inode_from_file(f->inum, fp);
    } else {
        f->inum = NULL;
    }

    fread(&f->num_children, sizeof(int), 1, fp);
    fread(&f->num_links, sizeof(int), 1, fp);
    fread(f->type, sizeof(char), 20, fp);
    
    
    if (f->num_children > 0) {
        f->children = malloc(f->num_children * sizeof(filetype*));
        if (!f->children) {
            fprintf(stderr, "Failed to allocate memory for children array\n");
            free(f->inum); 
            free(f); 
            return NULL;
        }

        for (int i = 0; i < f->num_children; i++) {
            f->children[i] = deserialize_filetype(fp, f);
            if (!f->children[i]) {
                for (int j = 0; j < i; j++) {
                    free(f->children[j]);
                }
                free(f->children);
                free(f->inum);
                free(f);
                return NULL;
            }
        }
    } else {
        f->children = NULL;
    }

    f->parent = parent;

    return f;
}


bool load_file_structure(const char *file_struct_path) {
    FILE *fp = fopen(file_struct_path, "rb");
    if (!fp) {
        perror("Failed to open file structure file");
        return false;
    }

    root = deserialize_filetype(fp, NULL);
    if (!root) {
        fclose(fp);
        return false;
    }

    fclose(fp);
    return true;
}



bool check_superblock_integrity() {
    print_debug("\n=================== Starting Superblock Integrity Check =================== \n");
    int error_count = 0;

    print_debug("[1/3] Checking data blocks size... ");
    size_t expected_data_size = block_size * 100;
    if (sizeof(s_block.data_blocks) != expected_data_size) {
        print_debug("FAIL (expected %zu, got %zu)\n", 
                  expected_data_size, sizeof(s_block.data_blocks));
        error_count++;
    } else {
        print_debug("OK\n");
    }

    print_debug("[2/3] Checking bitmap sizes... ");
    bool bitmaps_valid = true;
    
    if (sizeof(s_block.data_bitmap) != 105) {
        print_debug("\n  Data bitmap size mismatch (expected 105, got %zu)", 
                  sizeof(s_block.data_bitmap));
        bitmaps_valid = false;
    }
    
    if (sizeof(s_block.inode_bitmap) != 105) {
        print_debug("\n  Inode bitmap size mismatch (expected 105, got %zu)", 
                  sizeof(s_block.inode_bitmap));
        bitmaps_valid = false;
    }
    
    if (bitmaps_valid) {
        print_debug("OK\n");
    } else {
        error_count++;
    }

  

    print_debug("[3/3] Verifying bitmap consistency... ");
    int bitmap_errors = 0;
    
    for (int i = 0; i < 100; i++) {
        int byte = i / 8;
        int bit = i % 8;
        bool allocated = s_block.data_bitmap[byte] & (1 << bit);
        
        if (allocated && memset(&s_block.data_blocks[i*block_size], 0, block_size) == 0) {
            if (bitmap_errors == 0) print_debug("\n");
            print_debug("  Data block %d: marked but empty\n", i);
            bitmap_errors++;
        }
    }
    
    if (bitmap_errors > 0) {
        print_debug("  Found %d bitmap inconsistencies\n", bitmap_errors);
        error_count++;
    } else {
        print_debug("OK\n");
    }


    if (error_count == 0) {
        print_debug("\n=== Superblock Check PASSED ===\n");
        return true;
    } else {
        print_debug("\n=== Superblock Check FAILED (%d errors) ===\n", error_count);
        return false;
    }
}




bool check_filetype_node(filetype *node, int depth) {
    if (node == NULL) {
        print_debug("%*s[ERROR] Null node encountered\n", depth*2, "");
        return false;
    }

    print_debug("%*sChecking node '%s' (type: %s, valid: %d)\n", 
               depth*2, "", node->name, node->type, node->valid);

    if (node->valid != 1) {
        print_debug("%*s[ERROR] Invalid node state (valid=%d)\n", depth*2, "", node->valid);
        return false;
    }

    if (strlen(node->name) == 0) {
        print_debug("%*s[ERROR] Empty name\n", depth*2, "");
        return false;
    }

    if (strlen(node->path) == 0 && strcmp(node->name, "/") != 0) {
        print_debug("%*s[ERROR] Empty path for non-root node\n", depth*2, "");
        return false;
    }

    if (node->num_children < 0) {
        print_debug("%*s[ERROR] Invalid children count (%d)\n", depth*2, "", node->num_children);
        return false;
    }

    if ((node->num_children > 0 && node->children == NULL) || 
        (node->num_children == 0 && node->children != NULL)) {
        print_debug("%*s[ERROR] Children array mismatch (count=%d, array=%s)\n", 
                   depth*2, "", node->num_children, 
                   node->children ? "exists" : "null");
        return false;
    }

    if (node->parent == NULL && strcmp(node->name, "/") != 0) {
        print_debug("%*s[ERROR] Non-root node without parent\n", depth*2, "");
        return false;
    }

    bool all_children_valid = true;
    for (int i = 0; i < node->num_children; i++) {
        if (node->children[i] == NULL) {
            print_debug("%*s[ERROR] Null child at index %d\n", depth*2, "", i);
            all_children_valid = false;
            continue;
        }

        if (node->children[i]->parent != node) {
            print_debug("%*s[ERROR] Parent mismatch for child '%s'\n", 
                       depth*2, "", node->children[i]->name);
            all_children_valid = false;
        }

        if (!check_filetype_node(node->children[i], depth + 1)) {
            all_children_valid = false;
        }
    }

    return all_children_valid;
}

bool check_file_structure_integrity() {
    print_debug("\n======================== Starting File Structure Integrity Check ======================== \n");

    if (root == NULL) {
        print_debug("[CRITICAL] Root node is null\n");
        return false;
    }

    if (strcmp(root->name, "/") != 0) {
        print_debug("[ERROR] Root node name is not '/'\n");
        return false;
    }

    if (root->parent != NULL) {
        print_debug("[ERROR] Root node has non-null parent\n");
        return false;
    }

    print_debug("Checking root directory structure...\n");
    bool result = check_filetype_node(root, 0);

    if (result) {
        print_debug("\n=== File Structure Check PASSED ===\n");
    } else {
        print_debug("\n=== File Structure Check FAILED ===\n");
    }

    return result;
}



bool check_inode_integrity(inode *node) {
    if (node == NULL) {
        print_debug("\n[INODE CHECK] ERROR: Null inode pointer\n");
        return false;
    }

    print_debug("\n================================== Checking inode %d ==================================\n", node->number);


    print_debug("[1/6] Checking inode number... ");
    if (node->number < 0) {
        print_debug("FAIL (Invalid number: %d)\n", node->number);
        return false;
    }
    print_debug("OK (%d)\n", node->number);

    print_debug("[2/6] Checking block count... ");
    if (node->blocks < 0 || node->blocks > 16) {
        print_debug("FAIL (Invalid count: %d)\n", node->blocks);
        return false;
    }
    print_debug("OK (%d blocks)\n", node->blocks);


     print_debug("[3/6] Checking timestamps...\n");
    time_t current_time = time(NULL);
    
    print_debug("  - Access time: %ld ", node->a_time);
    if (node->a_time < 0 || node->a_time > current_time) {
        print_debug("- INVALID\n");
        return false;
    }
    print_debug("- OK\n");


    print_debug("  - Modification time: %ld ", node->m_time);
    if (node->m_time < 0 || node->m_time > current_time) {
        print_debug("- INVALID\n");
        return false;
    }
    print_debug("- OK\n");


    print_debug("  - Change time: %ld ", node->c_time);
    if (node->c_time < 0 || node->c_time > current_time) {
        print_debug("- INVALID\n");
        return false;
    }
    print_debug("- OK\n");
    
    print_debug("  - Birth time: %ld ", node->b_time);
    if (node->b_time < 0 || node->b_time > current_time) {
        print_debug("- INVALID\n");
        return false;
    }
    print_debug("- OK\n");

   print_debug("[4/6] Checking data blocks...\n");
    for (int i = 0; i < node->blocks; i++) {
        print_debug("  - Block %d: %d ", i, node->datablocks[i]);
        
        if (node->datablocks[i] < 0 || (size_t)node->datablocks[i] >= (block_size * 100)) {
            print_debug("- INVALID (Out of range)\n");
            return false;
        }
        
        print_debug("- OK\n");
    }

    print_debug("[5/6] Checking permissions... ");
    if (node->permissions > 40777) { 
        print_debug("FAIL (Invalid permissions: %04o)\n", node->permissions);    
        return false;
    }
    print_debug("OK (%04o)\n", node->permissions);

    print_debug("[6/6] Checking user/group IDs... ");
    if (node->user_id > UINT_MAX || node->group_id > UINT_MAX) {
        print_debug("FAIL (UID: %u, GID: %u)\n", node->user_id, node->group_id);
        return false;
    }
    print_debug("OK (UID: %u, GID: %u)\n", node->user_id, node->group_id);

    print_debug("=== Inode %d check COMPLETE - VALID ===\n\n", node->number);
    return true;
}



bool check_all_inodes(filetype *node) {
    if (node == NULL) {
        return true;
    }

    if (node->inum != NULL) {
        if (!check_inode_integrity(node->inum)) {
            return false;
        }
    }

    for (int i = 0; i < node->num_children; i++) {
        if (!check_all_inodes(node->children[i])) {
            return false;
        }
    }

    return true;
}

bool check_inode_integrity_in_filesystem() {
    printf("Checking inode integrity...\n");

    if (root == NULL) {
        printf("Error: Root file structure is null.\n");
        return false;
    }

    if (!check_all_inodes(root)) {
        printf("Error: Inode integrity check failed.\n");
        return false;
    }

    printf("Inode integrity check passed.\n");
    return true;
}


void check_filesystem() {
    printf("\nStarting filesystem check...\n");
    sleep(3);
    bool super_ok = check_superblock_integrity();
    sleep(3);
    bool struct_ok = check_file_structure_integrity();
    sleep(3);
    bool inodes_ok = check_inode_integrity_in_filesystem();
    
    if (debug_mode) {
        printf("\n=== SUMMARY ===\n");
        printf("Superblock: %s\n", super_ok ? "OK" : "FAILED");
        printf("File structure: %s\n", struct_ok ? "OK" : "FAILED");
        printf("Inodes: %s\n", inodes_ok ? "OK" : "FAILED");
    }
    
    if (super_ok && struct_ok && inodes_ok) {
        printf("\nFilesystem is healthy!\n");
    } else {
        printf("\nFilesystem has errors!\n");
    }
}


