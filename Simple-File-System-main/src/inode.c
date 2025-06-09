#include "../include/inode.h"

int find_free_inode() {
    for (int i = 2; i < 100; i++) {
        if (s_block.inode_bitmap[i] == '0') {
            s_block.inode_bitmap[i] = '1';
            return i; // Free inode index found, return it
        }
    }
    return -1; // No free inode found
}

void add_child(filetype *parent, filetype *child) {
    if (parent == NULL || child == NULL) {
        return; // Защита от NULL-указателей
    }

    // Увеличиваем счетчик детей
    parent->num_children++;

    // Попытка перевыделить память
    filetype **new_children = realloc(parent->children, parent->num_children * sizeof(filetype *));
    if (new_children == NULL) {
        // Если realloc не удался, откатываем num_children и выходим.
        // parent->children остается без изменений, но нового ребенка добавить не удалось.
        parent->num_children--;
        perror("Failed to reallocate memory for children array");
        return;
    }

    // Присваиваем новый указатель на массив
    parent->children = new_children;
    // Добавляем нового ребенка в конец массива
    parent->children[parent->num_children - 1] = child;
}
