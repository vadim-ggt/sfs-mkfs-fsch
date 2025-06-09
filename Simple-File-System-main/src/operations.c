#define _POSIX_C_SOURCE 200809L
#include <limits.h>
#include "../include/operations.h"
#include <fuse/fuse_lowlevel.h>  
#include <sys/stat.h>

struct fuse_operations operations =
{
    .mkdir=sfs_mkdir,
    .create=sfs_create,
    .rmdir=sfs_rmdir,
    .unlink=sfs_rm,
    .readdir=sfs_readdir,
    .getattr=sfs_getattr,
    .open=sfs_open,
    .write=sfs_write,
    .read=sfs_read,
    .rename=sfs_rename,
    .utimens=sfs_utimens,
    .release=sfs_release,
    .destroy = sfs_destroy,
    .truncate = sfs_truncate,
};

int sfs_mkdir(const char *path, mode_t mode) {
    (void) mode; // Explicitly cast unused parameter to void to avoid warning

    printf("Creating directory: %s\n", path);

    // Find a free inode
    int index = find_free_inode();
    if (index == -1) {
        return -ENOSPC; // No space left on device
    }

    // Allocate memory for the new folder
    filetype *new_folder = malloc(sizeof(filetype));
    if (new_folder) {
        memset(new_folder, 0, sizeof(filetype));
    }

    strcpy(new_folder->name, "");
    strcpy(new_folder->path, "");
    strcpy(new_folder->type, "");

    new_folder->inum = calloc(1, sizeof(inode));

    // Copy the path and extract the folder name
    char *pathname = malloc(strlen(path) + 2);
    strcpy(pathname, path);
    char *folder_name = strrchr(pathname, '/');
    if (folder_name != NULL) { // Check if strrchr found '/'
        strcpy(new_folder->name, folder_name + 1);
        *folder_name = '\0'; // Terminate the pathname string to remove the folder name
    }

    // Set the folder path
    if (strlen(pathname) == 0) {
        strcpy(pathname, "/");
    }
    strcpy(new_folder->path, pathname);

    // Get the parent folder
    new_folder->parent = filetype_from_path(pathname);
    if (new_folder->parent == NULL) {
        free(new_folder->inum);
        free(new_folder);
        free(pathname);
        return -ENOENT;
    }

    new_folder->children = NULL;

    // Add the new folder as a child of the parent folder
    add_child(new_folder->parent, new_folder);

    // Set the folder properties
    new_folder->num_children = 0;
    new_folder->num_links = 2;
    new_folder->valid = 1;
    strcpy(new_folder->type, "directory");

    // Set the inode properties
    inode *new_inode = new_folder->inum;
    new_inode->c_time = time(NULL);
    new_inode->a_time = time(NULL);
    new_inode->m_time = time(NULL);
    new_inode->b_time = time(NULL);
    new_inode->permissions = S_IFDIR | 0777;
    new_inode->size = 0;
    new_inode->group_id = getgid();
    new_inode->user_id = getuid();
    new_inode->number = index;
    new_inode->blocks = 0;

    save_system_state();

    free(pathname);

    return 0;
}

int sfs_getattr(const char *path, struct stat *stat_buf) {
    printf("Getting attributes for: %s\n", path);

    if (path == NULL || stat_buf == NULL) {
        return -EINVAL;
    }

    char *pathname = malloc(strlen(path) + 1);
    if (pathname == NULL) {
        return -ENOMEM;
    }
    strcpy(pathname, path);

    filetype *file_node = filetype_from_path(pathname);
    free(pathname);

    if (file_node == NULL) {
        return -ENOENT;
    }

    inode *file_inode = file_node->inum;
    if (file_inode == NULL) {
        return -ENOENT;
    }

    stat_buf->st_uid = file_inode->user_id;
    stat_buf->st_gid = file_inode->group_id;
    stat_buf->st_atime = file_inode->a_time;
    stat_buf->st_mtime = file_inode->m_time;
    stat_buf->st_ctime = file_inode->c_time;

    if (strcmp(file_node->type, "file") == 0) {
        stat_buf->st_mode = S_IFREG | file_inode->permissions;
    } else if (strcmp(file_node->type, "directory") == 0) {
        stat_buf->st_mode = S_IFDIR | file_inode->permissions;
    } else {
        stat_buf->st_mode = file_inode->permissions;
    }

    stat_buf->st_nlink = file_node->num_links + file_node->num_children;
    stat_buf->st_size = file_inode->size;
    stat_buf->st_blocks = file_inode->blocks;

    return 0;
}

int sfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    printf("Reading directory: %s\n", path);

    // Explicitly cast unused parameters to void to avoid warnings
    (void) offset;
    (void) fi;

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    // Allocate memory for the pathname with correct size
    char *pathname = malloc(strlen(path) + 1);
    if (pathname == NULL) {
        return -ENOMEM; // Not enough space
    }
    strcpy(pathname, path);

    filetype *dir_node = filetype_from_path(pathname);
    free(pathname); // Free pathname after use

    if (dir_node == NULL) {
        return -ENOENT; // No such file or directory
    }

    dir_node->inum->a_time = time(NULL); // Update access time

    for (int i = 0; i < dir_node->num_children; i++) {
        printf(":%s:\n", dir_node->children[i]->name);
        filler(buffer, dir_node->children[i]->name, NULL, 0);
    }

    return 0;
}

int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    printf("Creating file: %s\n", path);

    char *pathname = strdup(path);
    if (!pathname) return -ENOMEM;

    if (filetype_from_path(pathname) != NULL) {
        free(pathname);
        return -EEXIST;
    }

    int index = find_free_inode();
    if (index == -1) {
        free(pathname);
        return -ENOSPC;
    }

    filetype *new_file = calloc(1, sizeof(filetype));
    if (!new_file) {
        free(pathname);
        return -ENOMEM;
    }

    char *folder_name = strrchr(pathname, '/');
    if (!folder_name) {
        free(new_file);
        free(pathname);
        return -EINVAL;
    }

    strcpy(new_file->name, folder_name + 1);
    *folder_name = '\0';

    if (strlen(pathname) == 0) strcpy(pathname, "/");
    strcpy(new_file->path, pathname);

    new_file->parent = filetype_from_path(pathname);
    if (!new_file->parent) {
        free(new_file);
        free(pathname);
        return -ENOENT;
    }

    new_file->num_children = 0;
    add_child(new_file->parent, new_file);
    strcpy(new_file->type, "file");

    inode *new_inode = calloc(1, sizeof(inode));
    if (!new_inode) {
        // Убедитесь, что remove_child корректно удаляет filetype из списка детей родителя
        // и free_filetype освобождает new_file
        remove_child(new_file->parent, new_file);
        free(new_file); // Освобождаем new_file, так как он был выделен
        free(pathname);
        return -ENOMEM;
    }

    new_inode->number = index;
    new_inode->blocks = 0;
    new_inode->size = 0;
    new_inode->permissions = S_IFREG | (mode & 0777);
    new_inode->user_id = getuid();
    new_inode->group_id = getgid();
    time_t now = time(NULL);
    new_inode->a_time = new_inode->m_time = new_inode->c_time = new_inode->b_time = now;

    for (int i = 0; i < MAX_BLOCKS; i++) {
        new_inode->datablocks[i] = -1;
    }

    new_file->inum = new_inode;
    new_file->valid = 1;

    // Ключевое изменение: устанавливаем fi->fh здесь для случая, когда create объединяет open
    fi->fh = (uint64_t)new_file;
    printf("sfs_create: New file %s created and fi->fh set to %llu.\n", path, (unsigned long long)fi->fh);

    save_system_state();
    free(pathname);
    return 0;
}


int sfs_rmdir(const char *path) {
    printf("Removing directory: %s\n", path);

    char *pathname = strdup(path); // Используем strdup для безопасности и простоты
    if (!pathname) {
        return -ENOMEM;
    }

    char *folder_name_ptr = strrchr(pathname, '/');
    if (!folder_name_ptr) {
        free(pathname);
        return -EINVAL;
    }

    // ИСПРАВЛЕНИЕ: Используем strdup для корректного выделения памяти
    char *folder_delete = strdup(folder_name_ptr + 1);
    if (!folder_delete) {
        free(pathname);
        return -ENOMEM;
    }
    *folder_name_ptr = '\0'; // Обрезаем pathname, чтобы получить путь к родителю

    if (strlen(pathname) == 0) {
        strcpy(pathname, "/");
    }

    filetype *parent = filetype_from_path(pathname);
    free(pathname); // Освобождаем pathname как можно раньше

    if (parent == NULL) {
        free(folder_delete);
        return -ENOENT;
    }

    int index = 0;
    while (index < parent->num_children) {
        if (strcmp(parent->children[index]->name, folder_delete) == 0) {
            break;
        }
        index++;
    }
    free(folder_delete); // Освобождаем folder_delete

    if (index == parent->num_children) {
        return -ENOENT;
    }
    if (parent->children[index]->num_children != 0) {
        return -ENOTEMPTY;
    }

    free_filetype(parent->children[index]); // Ваша функция free_filetype должна освобождать inode и сам filetype

    for (int i = index + 1; i < parent->num_children; i++) {
        parent->children[i - 1] = parent->children[i];
    }
    parent->num_children -= 1;

    save_system_state();

    return 0;
}


int sfs_rm(const char *path) {
    printf("Removing file: %s\n", path);

    char *pathname = strdup(path);
    if (!pathname) return -ENOMEM;

    char *folder_name_ptr = strrchr(pathname, '/');
    if (!folder_name_ptr) {
        free(pathname);
        return -EINVAL;
    }

    // ИСПРАВЛЕНИЕ: Используем strdup для корректного выделения памяти
    char *file_delete = strdup(folder_name_ptr + 1);
    if (!file_delete) {
        free(pathname);
        return -ENOMEM;
    }
    *folder_name_ptr = '\0'; // Обрезаем pathname

    if (strlen(pathname) == 0) {
        strcpy(pathname, "/");
    }

    filetype *parent = filetype_from_path(pathname);
    free(pathname);

    if (parent == NULL) {
        free(file_delete);
        return -ENOENT;
    }

    int index = 0;
    while (index < parent->num_children) {
        if (strcmp(parent->children[index]->name, file_delete) == 0) {
            break;
        }
        index++;
    }
    free(file_delete);

    if (index == parent->num_children) {
        return -ENOENT;
    }

    if (strcmp(parent->children[index]->type, "directory") == 0) {
        return -EISDIR;
    }

    if (parent->children[index]->inum) {
        for (int i = 0; i < parent->children[index]->inum->blocks; i++) {
            if (parent->children[index]->inum->datablocks[i] != -1) {
                s_block.data_bitmap[parent->children[index]->inum->datablocks[i]] = '0';
            }
        }
        free(parent->children[index]->inum);
    }
    free(parent->children[index]);

    for (int i = index + 1; i < parent->num_children; i++) {
        parent->children[i - 1] = parent->children[i];
    }
    parent->num_children--;

    save_system_state();

    return 0;
}

int sfs_open(const char *path, struct fuse_file_info *fi) {
    printf("Opening file: %s\n", path);

    char *pathname = strdup(path);
    if (!pathname) {
        return -ENOMEM;
    }

    filetype *file = filetype_from_path(pathname);
    free(pathname);

    if (file == NULL) {
        printf("sfs_open: ERROR: File %s not found.\n", path);
        return -ENOENT;
    }
    if (strcmp(file->type, "directory") == 0) {
        printf("sfs_open: ERROR: Attempted to open directory %s as a file.\n", path);
        return -EISDIR;
    }

    fi->fh = (uint64_t)file;
    printf("sfs_open: File %s opened, fi->fh set to %llu (address of filetype).\n", path, (unsigned long long)fi->fh);
    printf("sfs_open: Flags: 0x%x\n", fi->flags); // Добавляем логирование флагов

    // Проверяем O_TRUNC, если файл открыт для записи и должен быть обрезан
    if ((fi->flags & O_ACCMODE) != O_RDONLY && (fi->flags & O_TRUNC)) {
        printf("sfs_open: O_TRUNC flag detected. Truncating file: %s\n", path);
        if (file->inum != NULL) {
            for (int i = 0; i < file->inum->blocks; i++) {
                if (file->inum->datablocks[i] != -1) {
                    s_block.data_bitmap[file->inum->datablocks[i]] = '0'; // Освобождаем блок
                    file->inum->datablocks[i] = -1; // Обнуляем указатель в иноде
                }
            }
            file->inum->size = 0;
            file->inum->blocks = 0;
            time_t now = time(NULL);
            file->inum->m_time = now;
            file->inum->c_time = now;
        } else {
            printf("sfs_open: WARNING: Attempted to truncate file '%s' with NULL inode.\n", path);
        }
    }

    if (file->inum != NULL) {
        file->inum->a_time = time(NULL);
    }

    save_system_state();

    return 0;
}

int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    printf("Reading file: %s, Size: %zu, Offset: %lld\n", path, size, (long long)offset);

    filetype *file = (filetype *)fi->fh;
    if (file == NULL || file->inum == NULL) {
        printf("sfs_read: ERROR: Invalid file handle (fi->fh is NULL or inode is NULL) for %s.\n", path);
        // Резервный вариант, если fi->fh не был установлен.
        char *pathname_backup = strdup(path);
        if (pathname_backup) {
            file = filetype_from_path(pathname_backup);
            free(pathname_backup);
        }
        if (file == NULL || file->inum == NULL) {
            printf("sfs_read: Critical Error: Could not recover filetype for %s. Returning -EIO.\n", path);
            return -EIO;
        }
        printf("sfs_read: Recovered filetype for %s via path lookup.\n", path);
    }

    printf("sfs_read: File found: %s, current size: %d, blocks: %d\n", file->name, file->inum->size, file->inum->blocks);

    if (strcmp(file->type, "directory") == 0) {
        printf("sfs_read: ERROR: Attempted to read from directory %s.\n", path);
        return -EISDIR;
    }

    if (offset >= (off_t)file->inum->size) {
        printf("sfs_read: Offset %lld >= file size %d. Returning 0 bytes read.\n", (long long)offset, file->inum->size);
        return 0;
    }

    size_t bytes_to_read_size_t = size;
    if (offset + bytes_to_read_size_t > (size_t)file->inum->size) {
        bytes_to_read_size_t = file->inum->size - offset;
    }

    if (bytes_to_read_size_t == 0) {
        printf("sfs_read: Calculated bytes to read is 0. Returning 0.\n");
        return 0;
    }

    file->inum->a_time = time(NULL);

    ssize_t current_read_offset = 0; // Используем ssize_t для счетчика прочитанных байт

    while (current_read_offset < (ssize_t)bytes_to_read_size_t) {
        int current_block_idx_in_inode = (offset + current_read_offset) / block_size;
        size_t current_offset_in_block = (offset + current_read_offset) % block_size;

        if (current_block_idx_in_inode >= MAX_BLOCKS || file->inum->datablocks[current_block_idx_in_inode] == -1) {
            printf("sfs_read: WARNING: Attempted to read from unallocated block at inode index %d for file %s. Only %zd bytes were readable.\n", current_block_idx_in_inode, path, current_read_offset);
            break; // Если блок не выделен, прекращаем чтение
        }

        int data_block_num = file->inum->datablocks[current_block_idx_in_inode];
        size_t bytes_left_in_current_block = block_size - current_offset_in_block;
        size_t bytes_to_copy_this_iter = bytes_to_read_size_t - current_read_offset;

        if (bytes_to_copy_this_iter > bytes_left_in_current_block) {
            bytes_to_copy_this_iter = bytes_left_in_current_block;
        }

        memcpy(buf + current_read_offset,
               s_block.data_blocks + data_block_num * block_size + current_offset_in_block,
               bytes_to_copy_this_iter);

        current_read_offset += bytes_to_copy_this_iter;
    }

    save_system_state();
    printf("sfs_read: Successfully read %zd bytes from file %s. Total read: %zd.\n", current_read_offset, path, current_read_offset);
    return (int)current_read_offset; // Приводим к int при возврате
}

int sfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    printf("Writing to file: %s, Size: %zu, Offset: %lld\n", path, size, (long long)offset);

    // Добавляем логирование содержимого буфера для отладки
    char debug_buf[64]; // Буфер для отладки, достаточно большой, но не слишком
    size_t debug_len = (size < sizeof(debug_buf) - 1) ? size : sizeof(debug_buf) - 1;
    memcpy(debug_buf, buf, debug_len);
    debug_buf[debug_len] = '\0';
    printf("sfs_write: buf_start='%s', size=%zu, offset=%lld\n",
           debug_buf, size, (long long)offset);


    filetype *file = (filetype *)fi->fh;
    if (file == NULL || file->inum == NULL) {
        printf("sfs_write: ERROR: Invalid file handle (fi->fh is NULL or inode is NULL) for %s.\n", path);
        // Резервный вариант, если fi->fh не был установлен (например, если FUSE вызвал create без open)
        char *pathname_backup = strdup(path);
        if (pathname_backup) {
            file = filetype_from_path(pathname_backup);
            free(pathname_backup);
        }
        if (file == NULL || file->inum == NULL) {
            printf("sfs_write: Critical Error: Could not recover filetype for %s. Returning -EIO.\n", path);
            return -EIO;
        }
        printf("sfs_write: Recovered filetype for %s via path lookup.\n", path);
    }

    printf("sfs_write: File found: %s, current size: %d, blocks: %d\n", file->name, file->inum->size, file->inum->blocks);

    if (strcmp(file->type, "directory") == 0) {
        printf("sfs_write: ERROR: Attempted to write to directory %s.\n", path);
        return -EISDIR;
    }

    if (size == 0) {
        printf("sfs_write: Zero size write, returning 0.\n");
        return 0;
    }

    if ((unsigned long long)offset + size > MAX_FILE_SIZE) {
        printf("sfs_write: ERROR: Attempted write exceeds MAX_FILE_SIZE (%lu bytes) for file %s.\n",
               (unsigned long)MAX_FILE_SIZE, path);
        return -EFBIG; // File too large
    }

    time_t now = time(NULL);
    file->inum->m_time = now;
    file->inum->a_time = now;

    ssize_t remaining_bytes_to_write = size;
    ssize_t bytes_written_total = 0;

    // Расширяем файл, если offset больше текущего размера. Это создает "дырку" (sparse file).
    if (offset > (off_t)file->inum->size) {
        printf("sfs_write: INFO: Offset %lld is beyond current file size %d. Updating file size to %lld.\n",
               (long long)offset, file->inum->size, (long long)offset);
        file->inum->size = offset;
    }

    while (remaining_bytes_to_write > 0) {
        int current_block_idx_in_inode = (offset + bytes_written_total) / block_size;
        size_t current_offset_in_block = (offset + bytes_written_total) % block_size;

        if (current_block_idx_in_inode >= MAX_BLOCKS) {
            printf("sfs_write: ERROR: Exceeded MAX_BLOCKS (%d) for inode for file %s. Wrote %zd bytes so far.\n",
                   MAX_BLOCKS, path, bytes_written_total);
            save_system_state();
            return (int)bytes_written_total; // Возвращаем то, что успели записать
        }

        // Если текущий блок еще не выделен (т.е. это "дырка" или новый блок в конце файла)
        if (file->inum->datablocks[current_block_idx_in_inode] == -1) {
            int new_db_num = find_free_db();
            if (new_db_num == -1) {
                printf("sfs_write: ERROR: No free data blocks to allocate for file %s. Wrote %zd bytes so far.\n",
                       path, bytes_written_total);
                save_system_state();
                return -ENOSPC; // Возвращаем то, что успели записать
            }
            file->inum->datablocks[current_block_idx_in_inode] = new_db_num;
            s_block.data_bitmap[new_db_num] = '1'; // Отмечаем блок как занятый
            printf("sfs_write: Allocated new data block %d at inode index %d for file %s.\n",
                   new_db_num, current_block_idx_in_inode, path);

            // Если новый блок выделяется не с самого начала (например, если offset не кратен block_size
            // или если мы заполняем дырку в середине файла), нужно обнулить его часть,
            // которая не будет заполнена текущей записью.
            // Это обеспечивает, что не будет "мусора" при последующем чтении.
            // Исключение: если мы пишем точно с начала блока и до его конца, memset не нужен.
            if (current_offset_in_block != 0) {
                // Если запись начинается не с начала блока, обнуляем часть до смещения
                memset(s_block.data_blocks + new_db_num * block_size, 0, current_offset_in_block);
                printf("sfs_write: Initialized new data block %d (before offset %zu) with zeros.\n", new_db_num, current_offset_in_block);
            }
        }

        // Обновляем количество блоков в inode, если мы выделили блок за пределами текущего `blocks`
        if (current_block_idx_in_inode >= file->inum->blocks) {
            file->inum->blocks = current_block_idx_in_inode + 1;
        }

        int data_block_num_in_super = file->inum->datablocks[current_block_idx_in_inode];
        size_t space_left_in_current_block = block_size - current_offset_in_block;
        size_t bytes_to_copy_this_iter = (size_t)remaining_bytes_to_write;
        if (bytes_to_copy_this_iter > space_left_in_current_block) {
            bytes_to_copy_this_iter = space_left_in_current_block;
        }

        memcpy(s_block.data_blocks + data_block_num_in_super * block_size + current_offset_in_block,
               buf + bytes_written_total, bytes_to_copy_this_iter);

        bytes_written_total += bytes_to_copy_this_iter;
        remaining_bytes_to_write -= bytes_to_copy_this_iter;

        // Обновляем размер файла, если запись его увеличивает
        if ((off_t)(offset + bytes_written_total) > (off_t)file->inum->size) {
            file->inum->size = (offset + bytes_written_total);
        }
    }

    save_system_state();
    printf("sfs_write: Successfully wrote %zd bytes to file %s. New size: %d.\n", bytes_written_total, path, file->inum->size);
    return (int)bytes_written_total; // Приводим к int при возврате
}

int sfs_release(const char *path, struct fuse_file_info *fi) {
    printf("Releasing file: %s\n", path);
    save_system_state(); // Сохраняем состояние ФС при закрытии файла
    (void) path; // Отключаем предупреждение о неиспользуемом параметре
    (void) fi;   // Отключаем предупреждение о неиспользуемом параметре
    return 0;
}

int sfs_rename(const char *from, const char *to) {
    printf("Renaming file/directory from %s to %s\n", from, to);

    filetype *from_node = filetype_from_path(from);
    if (from_node == NULL) {
        return -ENOENT;
    }

    char *dest_name = get_file_name(to);
    char *path_to = get_file_path(to);
    char *path_from = get_file_path(from);
    char *from_name = get_file_name(from); // нужно заранее сохранить имя from

    filetype *to_dir = filetype_from_path(to);
    if (to_dir == NULL) {
        filetype *parent_dir_from = filetype_from_path(path_from);
        filetype *parent_dir_to = filetype_from_path(path_to);
        if (parent_dir_from == NULL || parent_dir_to == NULL) {
            free(dest_name);
            free(path_to);
            free(path_from);
            free(from_name);
            return -ENOENT;
        }

        for (int i = 0; i < parent_dir_from->num_children; i++) {
            if (strcmp(parent_dir_from->children[i]->name, from_name) == 0) {
                if (strcmp(dest_name, from_name) != 0) {
                    strcpy(parent_dir_from->children[i]->name, dest_name);
                }

                char *to_file_path = get_file_path(to); // временно
                strcpy(parent_dir_from->children[i]->path, to_file_path);
                free(to_file_path); // освобождаем

                add_child(parent_dir_to, parent_dir_from->children[i]);

                for (int j = i + 1; j < parent_dir_from->num_children; j++) {
                    parent_dir_from->children[j - 1] = parent_dir_from->children[j];
                }
                parent_dir_from->num_children--;

                break;
            }
        }

        printf("To - %s %s : From - %s %s\n", path_to, dest_name, path_from, from_name);

        save_system_state();

        // освобождаем память
        free(dest_name);
        free(path_to);
        free(path_from);
        free(from_name);

        return 0;
    }

    // освобождаем, если дошли сюда (файл уже существует)
    free(dest_name);
    free(path_to);
    free(path_from);
    free(from_name);

    return -EEXIST; // например, если переименование не удалось
}


int sfs_utimens(const char *path, const struct timespec tv[2]) {
    filetype *file = filetype_from_path(path);
    if (file == NULL) {
        return -ENOENT; // File not found
    }

    time_t currentTime = time(NULL); // Fetch current time once, if needed

    // Update access time
    if (tv[0].tv_nsec != UTIME_OMIT) {
        file->inum->a_time = (tv[0].tv_nsec == UTIME_NOW) ? currentTime : tv[0].tv_sec;
    }

    if (tv[1].tv_nsec != UTIME_OMIT) {
        file->inum->m_time = (tv[1].tv_nsec == UTIME_NOW) ? currentTime : tv[1].tv_sec;
    }

    save_system_state();

    return 0;
}

void sfs_destroy(void *private_data) {
    (void) private_data; // Отключаем предупреждение о неиспользуемом параметре
    printf("SFS: Destroying file system. Freeing all resources.\n");
    free_filetype(root); // Теперь это безопасное место для освобождения
    root = NULL; // Обнуляем указатель после освобождения
    // Освободите здесь любые другие глобальные ресурсы, если они есть.
    // Например, если s_block выделялся динамически, то free(s_block);
    // Но так как s_block - это глобальная переменная, она будет очищена при выходе из программы.
    // Главное - очистить все, что было выделено динамически.
}

int sfs_truncate(const char *path, off_t size) {
    printf("sfs_truncate: Truncating file %s to size %lld\n", path, (long long)size);

    // Временная реализация: найти файл и обрезать его
    filetype *file = filetype_from_path(path);
    if (file == NULL) {
        return -ENOENT;
    }
    if (strcmp(file->type, "directory") == 0) {
        return -EISDIR;
    }

    // Если размер 0, то освобождаем все блоки
    if (size == 0) {
        if (file->inum != NULL) {
            for (int i = 0; i < file->inum->blocks; i++) {
                if (file->inum->datablocks[i] != -1) {
                    s_block.data_bitmap[file->inum->datablocks[i]] = '0'; // Освобождаем блок
                    file->inum->datablocks[i] = -1; // Обнуляем указатель в иноде
                }
            }
            file->inum->size = 0;
            file->inum->blocks = 0;
            time_t now = time(NULL);
            file->inum->m_time = now;
            file->inum->c_time = now;
        }
    } else {
        // Если size > 0, то тут должна быть логика расширения или сжатия файла.
        // Пока что для теста можно просто вернуть 0 или -EINVAL.
        // В реальной ФС, если размер больше текущего, нужно выделить блоки.
        // Если меньше, освободить лишние.
        printf("sfs_truncate: WARNING: Truncating to non-zero size %lld is not fully implemented yet for %s.\n", (long long)size, path);
        return -EINVAL; // Временно, пока не реализуете
    }

    save_system_state();
    return 0;
}

