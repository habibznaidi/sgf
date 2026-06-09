#ifndef INODE_H
#define INODE_H

#include "fs.h"

int    alloc_inode(void);
void   free_inode(int n);
Inode *get_inode(int n);
void   init_inode(int n, int type, int permissions);
void   permissions_to_str(int type, int perms, char *out);

#endif
