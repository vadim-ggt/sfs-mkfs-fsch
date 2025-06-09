#include "../include/fs_init.h"
// gcc shell.c -o FS `pkg-config fuse --cflags --libs`

//MAIN FS
// ./shell -f /home/alexander/mnt
// fusermount -u /home/alexander/mnt

//SDCARD
// sudo mount /dev/mmcblk0p1 /home/alexander/sdCard/
// sudo ./shell -f -o allow_other /home/alexander/sdCard/mnt/
// sudo fusermount -u /home/alexander/sdCard/mnt/

#include <stdio.h>
#include "../include/fs_init.h"
#include "../include/operations.h"

int main(int argc, char *argv[]) {
    restore_file_system();

    int ret = fuse_main(argc, argv, &operations, NULL);


    root = NULL;

    return ret;
}
