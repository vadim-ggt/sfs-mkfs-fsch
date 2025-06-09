#include "../include/filetype.h"

filetype *filetype_from_path(const char *path) {
    if (path == NULL) {
        printf("NULL path provided.\n");
        return NULL; // Handle null pointer to avoid undefined behavior.
    }

    if (path[0] != '/' || strcmp(path, "/") == 0) {
        return root; // Simplify the check for root path.
    }

    char *path_name = strdup(path); // Use strdup to simplify memory allocation and copying.
    if (!path_name) {
        perror("Memory allocation failed for path_name");
        return NULL; // Check strdup failure.
    }

    if (path_name[strlen(path_name) - 1] == '/') {
        path_name[strlen(path_name) - 1] = '\0'; // Remove trailing slash if present.
    }

    filetype *curr_node = root;
    char *token = strtok(path_name + 1, "/"); // Skip the leading '/' and tokenize.

    while (token != NULL) {
        int found = 0;
        for (int i = 0; i < curr_node->num_children && !found; i++) {
            if (strcmp(curr_node->children[i]->name, token) == 0) {
                curr_node = curr_node->children[i];
                found = 1; // Use a boolean flag to indicate if the child was found.
            }
        }

        if (!found) {
            free(path_name);
            return NULL; // Child not found.
        }

        token = strtok(NULL, "/"); // Get the next token.
    }

    free(path_name);
    return curr_node;
}