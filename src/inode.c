#include "../include/inode.h"

int find_free_inode() {
    for (int i = 2; i < 100; i++) {
        if (s_block.inode_bitmap[i] == '0') {
            s_block.inode_bitmap[i] = '1';
            return i; 
        }
    }
    return -1; 
}

void add_child(filetype *parent, filetype *child) {
    parent->num_children++;

    filetype **new_children = realloc(parent->children, parent->num_children * sizeof(filetype *));
    if (new_children == NULL) {
        parent->num_children--;
        return;
    }

    parent->children = new_children;
    parent->children[parent->num_children - 1] = child;
}