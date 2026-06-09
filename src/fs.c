/*
 * fs.c — Initialisation et formatage du SGF
 */

#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/block.h"
#include "../include/dir_ops.h"

FileSystem fs;

void fs_format(void) {
    int i, root_block;

    memset(&fs, 0, sizeof(FileSystem));

    fs.sb.total_blocks = MAX_BLOCKS;
    fs.sb.free_blocks  = MAX_BLOCKS;
    fs.sb.total_inodes = MAX_INODES;
    fs.sb.free_inodes  = MAX_INODES;
    fs.sb.root_inode   = ROOT_INODE;
    fs.sb.block_size   = BLOCK_SIZE;
    strncpy(fs.sb.label, "MiniSGF-ISTY", 31);

    for (i = 0; i < MAX_INODES; i++) {
        fs.inode_bitmap[i] = 0;
        fs.inodes[i].used  = 0;
        for (int j = 0; j < MAX_BLOCKS_PER_FILE; j++)
            fs.inodes[i].blocks[j] = -1;
    }
    for (i = 0; i < MAX_BLOCKS; i++) {
        fs.block_bitmap[i] = 0;
        memset(fs.blocks[i].data, 0, BLOCK_SIZE);
    }

    /* Inode racine */
    fs.inode_bitmap[ROOT_INODE]       = 1;
    fs.sb.free_inodes--;
    fs.inodes[ROOT_INODE].used        = 1;
    fs.inodes[ROOT_INODE].type        = TYPE_DIR;
    fs.inodes[ROOT_INODE].permissions = PERM_RWX;
    fs.inodes[ROOT_INODE].size        = 0;
    fs.inodes[ROOT_INODE].nlinks      = 2;
    fs.inodes[ROOT_INODE].mtime       = time(NULL);
    for (i = 0; i < MAX_BLOCKS_PER_FILE; i++)
        fs.inodes[ROOT_INODE].blocks[i] = -1;

    root_block = alloc_block();
    fs.inodes[ROOT_INODE].blocks[0] = root_block;

    DirEntry *entries = (DirEntry *) fs.blocks[root_block].data;
    for (i = 0; i < ENTRIES_PER_BLOCK; i++) {
        memset(entries[i].name, 0, MAX_FILENAME);
        entries[i].inode_num = -1;
    }
    strncpy(entries[0].name, ".", MAX_FILENAME - 1);
    entries[0].inode_num = ROOT_INODE;
    strncpy(entries[1].name, "..", MAX_FILENAME - 1);
    entries[1].inode_num = ROOT_INODE;
    fs.inodes[ROOT_INODE].size = 2 * DIR_ENTRY_SIZE;

    fs.current_dir = ROOT_INODE;
    strncpy(fs.current_path, "/", MAX_PATH - 1);

    printf("  Disque formatte : %d inodes, %d blocs x %d octets\n",
           MAX_INODES, MAX_BLOCKS, BLOCK_SIZE);
}

void fs_print_info(void) {
    int used_blocks  = fs.sb.total_blocks - fs.sb.free_blocks;
    int used_inodes  = fs.sb.total_inodes - fs.sb.free_inodes;
    int free_bytes   = fs.sb.free_blocks  * fs.sb.block_size;
    int total_bytes  = fs.sb.total_blocks * fs.sb.block_size;

    printf("\n");
    printf("  +-----------------------------------------------+\n");
    printf("  |  SGF : %-38s|\n", fs.sb.label);
    printf("  +-------------------+---------------------------+\n");
    printf("  | Taille d'un bloc  | %-5d octets             |\n", fs.sb.block_size);
    printf("  +-------------------+---------------------------+\n");
    printf("  | Blocs totaux      | %-5d                     |\n", fs.sb.total_blocks);
    printf("  | Blocs utilises    | %-5d                     |\n", used_blocks);
    printf("  | Blocs libres      | %-5d                     |\n", fs.sb.free_blocks);
    printf("  +-------------------+---------------------------+\n");
    printf("  | Inodes totaux     | %-5d                     |\n", fs.sb.total_inodes);
    printf("  | Inodes utilises   | %-5d                     |\n", used_inodes);
    printf("  | Inodes libres     | %-5d                     |\n", fs.sb.free_inodes);
    printf("  +-------------------+---------------------------+\n");
    printf("  | Espace total      | %-7d octets           |\n", total_bytes);
    printf("  | Espace libre      | %-7d octets           |\n", free_bytes);
    printf("  | Espace utilise    | %-7d octets           |\n", total_bytes - free_bytes);
    printf("  +-------------------+---------------------------+\n");
    printf("  | Repertoire actuel | %-27s|\n", fs.current_path);
    printf("  +-------------------+---------------------------+\n\n");
}
