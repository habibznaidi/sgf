#ifndef FILE_OPS_H
#define FILE_OPS_H

#include "fs.h"

#define MAX_OPEN_FILES 16

typedef struct {
    int used;
    int inode_num;
    int mode;
    int offset;
} OpenFile;

extern OpenFile open_files[MAX_OPEN_FILES];

int  mycreat(const char *name, int mode);
int  myopen(const char *name, int mode);
int  myclose(int fd);
int  myread(int fd, char *buffer, int nombre);
int  mywrite(int fd, const char *buffer, int nombre);
int  read_file_by_inode(int inode_num, char *buf, int buf_size);
int  write_file_by_inode(int inode_num, const char *buf, int len);
void init_open_files(void);

#endif
