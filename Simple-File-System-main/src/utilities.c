#include "../include/utilities.h"

void serialize_superblock_to_file(superblock *sb, FILE *fp) {
    fwrite(sb->data_blocks, sizeof(char), block_size * 100, fp);
    fwrite(sb->data_bitmap, sizeof(char), 105, fp);
    fwrite(sb->inode_bitmap, sizeof(char), 105, fp);
}

// Десериализация структуры superblock из файла
void deserialize_superblock_from_file(superblock *sb, FILE *fp) {
    fread(sb->data_blocks, sizeof(char), block_size * 100, fp);
    fread(sb->data_bitmap, sizeof(char), 105, fp);
    fread(sb->inode_bitmap, sizeof(char), 105, fp);
}

void serialize_inode_to_file(inode *i, FILE *fp) {
    fwrite(i->datablocks, sizeof(int), 16, fp);
    fwrite(&i->number, sizeof(int), 1, fp);
    fwrite(&i->blocks, sizeof(int), 1, fp);
    fwrite(&i->size, sizeof(int), 1, fp);
    fwrite(&i->permissions, sizeof(mode_t), 1, fp);
    fwrite(&i->user_id, sizeof(uid_t), 1, fp);
    fwrite(&i->group_id, sizeof(gid_t), 1, fp);
    fwrite(&i->a_time, sizeof(time_t), 1, fp);
    fwrite(&i->m_time, sizeof(time_t), 1, fp);
    fwrite(&i->c_time, sizeof(time_t), 1, fp);
    fwrite(&i->b_time, sizeof(time_t), 1, fp);
}

void deserialize_inode_from_file(inode *i, FILE *fp) {
    fread(i->datablocks, sizeof(int), 16, fp);
    fread(&i->number, sizeof(int), 1, fp);
    fread(&i->blocks, sizeof(int), 1, fp);
    fread(&i->size, sizeof(int), 1, fp);
    fread(&i->permissions, sizeof(mode_t), 1, fp);
    fread(&i->user_id, sizeof(uid_t), 1, fp);
    fread(&i->group_id, sizeof(gid_t), 1, fp);
    fread(&i->a_time, sizeof(time_t), 1, fp);
    fread(&i->m_time, sizeof(time_t), 1, fp);
    fread(&i->c_time, sizeof(time_t), 1, fp);
    fread(&i->b_time, sizeof(time_t), 1, fp);
}

void serialize_filetype_to_file(filetype *f, FILE *fp) {
    fwrite(&f->valid, sizeof(int), 1, fp);
    fwrite(f->path, sizeof(char), 100, fp);
    fwrite(f->name, sizeof(char), 100, fp);
    if (f->inum != NULL) {
        int null_flag = 1;
        fwrite(&null_flag, sizeof(int), 1, fp);

        serialize_inode_to_file(f->inum, fp);
    } else {
        int null_flag = 0;
        fwrite(&null_flag, sizeof(int), 1, fp);
    }
    fwrite(&f->num_children, sizeof(int), 1, fp);
    fwrite(&f->num_links, sizeof(int), 1, fp);
    fwrite(f->type, sizeof(char), 20, fp);
    if (f->num_children > 0) {
        for (int i = 0; i < f->num_children; i++) {
            serialize_filetype_to_file(f->children[i], fp);
        }
    }
}

void deserialize_filetype_from_file(filetype *f, FILE *fp) {
    if (!f || !fp) return;

    size_t bytes_read;

    bytes_read = fread(&f->valid, sizeof(int), 1, fp);
    if (bytes_read != 1) return; // Проверка чтения

    bytes_read = fread(f->path, sizeof(char), 100, fp);
    if (bytes_read != 100) return;
    f->path[99] = '\0';

    bytes_read = fread(f->name, sizeof(char), 100, fp);
    if (bytes_read != 100) return;
    f->name[99] = '\0';
    printf("Deserializing: %s at %s\n", f->name, f->path); // Теперь печатаем после загрузки

    int null_flag;
    bytes_read = fread(&null_flag, sizeof(int), 1, fp);
    if (bytes_read != 1) return;

    if (null_flag == 1) {
        inode *inum = calloc(1, sizeof(inode)); // Используем calloc для обнуления
        if (!inum) {
            perror("malloc failed for inode during deserialization");
            exit(EXIT_FAILURE); // Критическая ошибка, завершаем
        }
        deserialize_inode_from_file(inum, fp);
        f->inum = inum;
    } else {
        f->inum = NULL;
    }

    bytes_read = fread(&f->num_children, sizeof(int), 1, fp);
    if (bytes_read != 1) return;

    bytes_read = fread(&f->num_links, sizeof(int), 1, fp);
    if (bytes_read != 1) return;

    bytes_read = fread(f->type, sizeof(char), 20, fp);
    if (bytes_read != 20) return;
    f->type[19] = '\0';

    if (f->num_children > 0) {
        // Освобождаем старый массив детей, если он был (например, при повторном вызове)
        if (f->children != NULL) {
            // Если free_filetype уже вызывается рекурсивно, то это не нужно.
            // Но если вы хотите быть на 100% уверенным, что старые указатели не утекут,
            // то здесь нужно их освободить, прежде чем перевыделять.
            // Однако, в контексте восстановления из файла, f->children обычно NULL.
            // Но если root уже был создан и пересоздается, то тут проблема.
            // Самый безопасный путь - убедиться, что root очищается перед восстановлением.
        }
        f->children = calloc(f->num_children, sizeof(filetype*)); // Используем calloc для обнуления
        if (!f->children) {
            perror("malloc failed for children array during deserialization");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < f->num_children; i++) {
            filetype *child = calloc(1, sizeof(filetype)); // Используем calloc для обнуления
            if (!child) {
                perror("malloc failed for child during deserialization");
                exit(EXIT_FAILURE);
            }
            // Рекурсивный вызов для десериализации ребенка
            deserialize_filetype_from_file(child, fp);
            f->children[i] = child;
            child->parent = f; // Устанавливаем указатель на родителя! Это очень важно.
        }
    } else {
        // Если детей нет, убедимся, что children NULL, чтобы избежать висячих указателей
        if (f->children != NULL) {
            free(f->children);
            f->children = NULL;
        }
    }
}


char* get_file_name(const char* path) {
    const char* last_slash = strrchr(path, '/');
    const char* name = last_slash ? last_slash + 1 : path;

    char* result = malloc(strlen(name) + 1);
    if (result == NULL) {
        return NULL;
    }
    strcpy(result, name);
    return result;
}

char* get_file_path(const char* path) {
    const char* last_slash = strrchr(path, '/');
    if (last_slash == NULL || last_slash == path) {
        char* result = malloc(2);  // for "/\0"
        if (result) strcpy(result, "/");
        return result;
    }

    size_t len = last_slash - path;
    char* result = malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }
    strncpy(result, path, len);
    result[len] = '\0';
    return result;
}

void free_filetype(filetype *node) {
    if (!node) return;

    for (int i = 0; i < node->num_children; i++) {
        free_filetype(node->children[i]);  
    }

    if(node->inum!=NULL){free(node->inum);} 
    if(node->children!=NULL){free(node->children); } 
    if(node!=NULL){free(node);}      
}
