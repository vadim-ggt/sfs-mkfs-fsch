#include "../include/filetype.h"

filetype *filetype_from_path(const char *path) {
    if (path == NULL) {
        printf("NULL path provided.\n");
        return NULL;
    }

    // Если это корневой путь, сразу возвращаем root
    if (strcmp(path, "/") == 0) {
        return root;
    }

    // Используем временную копию пути, чтобы strtok не модифицировал оригинал
    char *path_copy = strdup(path);
    if (!path_copy) {
        perror("Memory allocation failed for path_copy");
        return NULL;
    }

    // Удаляем завершающий слэш, если есть (кроме случая с корнем, который уже обработан)
    size_t len = strlen(path_copy);
    if (len > 1 && path_copy[len - 1] == '/') {
        path_copy[len - 1] = '\0';
    }

    filetype *curr_node = root;
    char *token;

    // Начинаем токенизацию с первого символа после начального слэша
    // Например, для "/a/b", начнем с "a/b"
    token = strtok(path_copy + 1, "/");

    while (token != NULL) {
        int found = 0;
        // Проверяем children, но также убедимся, что curr_node->children не NULL
        if (curr_node->children != NULL) {
            for (int i = 0; i < curr_node->num_children; i++) {
                // Важная проверка на NULL для curr_node->children[i]
                if (curr_node->children[i] != NULL && strcmp(curr_node->children[i]->name, token) == 0) {
                    curr_node = curr_node->children[i];
                    found = 1;
                    break; // Найден, выходим из внутреннего цикла
                }
            }
        }

        if (!found) {
            free(path_copy);
            return NULL; // Дочерний элемент не найден
        }

        token = strtok(NULL, "/"); // Получаем следующий токен
    }

    free(path_copy); // Освобождаем выделенную память
    return curr_node;
}




void remove_child(filetype *parent, filetype *child) {
    for (int i = 0; i < parent->num_children; i++) {
        if (parent->children[i] == child) {
            for (int j = i; j < parent->num_children - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->num_children--;
            return;
        }
    }
}
