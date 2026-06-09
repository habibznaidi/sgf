#ifndef BLOCK_H
#define BLOCK_H

#include "fs.h"

int    alloc_block(void);
void   free_block(int n);
Block *get_block(int n);
void   free_inode_blocks(int inode_num);

#endif
