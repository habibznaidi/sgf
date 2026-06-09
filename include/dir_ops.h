#ifndef DIR_OPS_H
#define DIR_OPS_H

#include "fs.h"

int  dir_add_entry(int dir_inode, const char *name, int inode_num);
int  dir_find_entry(int dir_inode, const char *name);
int  dir_remove_entry(int dir_inode, const char *name);
int  dir_count_entries(int dir_inode);

int  fs_mkdir(const char *name);
int  fs_rmdir(const char *name);
int  fs_link(const char *name1, const char *name2);
int  fs_unlink(const char *name);

int  resolve_path(const char *path);
int  fs_cd(const char *path);
void fs_ls(int long_format);
void fs_ls_dir(int dir_inode, int long_format);
int  split_path(const char *path, int *parent_inode, char *last_comp);

#endif
