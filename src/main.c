/*
 * main.c — Point d'entrée du Mini SGF
 */

#include "../include/fs.h"
#include "../include/save.h"
#include "../include/file_ops.h"
#include "../include/shell.h"

int main(void) {
    init_open_files();
    if (load_fs(DISK_IMAGE) < 0) {
        printf("  Premier lancement : formatage du disque...\n");
        fs_format();
        save_fs(DISK_IMAGE);
    }
    run_shell();
    return 0;
}
