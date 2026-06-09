/*
 * block.c — Gestion des blocs de données
 */

#include "../include/fs.h"
#include "../include/block.h"

int alloc_block(void) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (fs.block_bitmap[i] == 0) {
            fs.block_bitmap[i] = 1;
            fs.sb.free_blocks--;
            memset(fs.blocks[i].data, 0, BLOCK_SIZE);
            return i;
        }
    }
    fprintf(stderr, "ERREUR : disque plein\n");
    return FS_ERR;
}

void free_block(int n) {
    if (n < 0 || n >= MAX_BLOCKS) return;
    memset(fs.blocks[n].data, 0, BLOCK_SIZE);
    fs.block_bitmap[n] = 0;
    fs.sb.free_blocks++;
}

Block *get_block(int n) {
    if (n < 0 || n >= MAX_BLOCKS) return NULL;
    return &fs.blocks[n];
}

void free_inode_blocks(int inode_num) {
    Inode *inode = &fs.inodes[inode_num];
    for (int j = 0; j < MAX_BLOCKS_PER_FILE; j++) {
        if (inode->blocks[j] != -1) {
            free_block(inode->blocks[j]);
            inode->blocks[j] = -1;
        }
    }
    inode->size = 0;
}
