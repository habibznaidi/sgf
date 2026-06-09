#ifndef FS_H
#define FS_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INODES          64
#define MAX_BLOCKS          256
#define BLOCK_SIZE          256
#define MAX_FILENAME        28
#define MAX_BLOCKS_PER_FILE 8
#define DIR_ENTRY_SIZE      32
#define ENTRIES_PER_BLOCK   (BLOCK_SIZE / DIR_ENTRY_SIZE)
#define MAX_PATH            512
#define DISK_IMAGE          "disk.img"
#define ROOT_INODE          0

#define TYPE_FREE  0
#define TYPE_FILE  1
#define TYPE_DIR   2

#define PERM_R    4
#define PERM_W    2
#define PERM_X    1
#define PERM_RW   6
#define PERM_RWX  7

#define MODE_READ    0
#define MODE_WRITE   1
#define MODE_APPEND  2

#define FS_OK    0
#define FS_ERR  -1

typedef struct {
    char name[MAX_FILENAME];
    int  inode_num;
} DirEntry;

typedef struct {
    int    used;
    int    type;
    int    permissions;
    int    size;
    int    blocks[MAX_BLOCKS_PER_FILE];
    int    nlinks;
    time_t mtime;
} Inode;

typedef struct {
    char data[BLOCK_SIZE];
} Block;

typedef struct {
    int  total_blocks;
    int  free_blocks;
    int  total_inodes;
    int  free_inodes;
    int  root_inode;
    int  block_size;
    char label[32];
} SuperBlock;

typedef struct {
    SuperBlock sb;
    Inode      inodes[MAX_INODES];
    Block      blocks[MAX_BLOCKS];
    int        inode_bitmap[MAX_INODES];
    int        block_bitmap[MAX_BLOCKS];
    int        current_dir;
    char       current_path[MAX_PATH];
} FileSystem;

extern FileSystem fs;

void fs_format(void);
void fs_print_info(void);

#endif
