/*
 * file_ops.c — Primitives fichiers : mycreat, myopen, myclose, myread, mywrite
 */

#include "../include/fs.h"
#include "../include/file_ops.h"
#include "../include/inode.h"
#include "../include/block.h"
#include "../include/dir_ops.h"

OpenFile open_files[MAX_OPEN_FILES];

void init_open_files(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        open_files[i].used      = 0;
        open_files[i].inode_num = -1;
        open_files[i].mode      = 0;
        open_files[i].offset    = 0;
    }
}

static int find_free_fd(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++)
        if (!open_files[i].used) return i;
    return FS_ERR;
}

/* ---- mycreat ---- */
int mycreat(const char *name, int mode) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_FILENAME) {
        fprintf(stderr, "mycreat: nom invalide\n");
        return FS_ERR;
    }
    int existing = dir_find_entry(fs.current_dir, name);
    if (existing != FS_ERR) return existing;

    int inode_num = alloc_inode();
    if (inode_num < 0) return FS_ERR;

    init_inode(inode_num, TYPE_FILE, mode != 0 ? mode : PERM_RW);
    fs.inodes[inode_num].nlinks = 1;

    if (dir_add_entry(fs.current_dir, name, inode_num) < 0) {
        free_inode(inode_num);
        return FS_ERR;
    }
    return inode_num;
}

/* ---- myopen ---- */
int myopen(const char *name, int mode) {
    int inode_num = dir_find_entry(fs.current_dir, name);
    if (inode_num < 0) {
        if (mode == MODE_READ) {
            fprintf(stderr, "myopen: '%s' introuvable\n", name);
            return FS_ERR;
        }
        inode_num = mycreat(name, PERM_RW);
        if (inode_num < 0) return FS_ERR;
    }
    if (fs.inodes[inode_num].type == TYPE_DIR) {
        fprintf(stderr, "myopen: '%s' est un repertoire\n", name);
        return FS_ERR;
    }
    int fd = find_free_fd();
    if (fd < 0) { fprintf(stderr, "myopen: trop de fichiers ouverts\n"); return FS_ERR; }

    open_files[fd].used      = 1;
    open_files[fd].inode_num = inode_num;
    open_files[fd].mode      = mode;
    open_files[fd].offset    = (mode == MODE_APPEND) ? fs.inodes[inode_num].size : 0;
    return fd;
}

/* ---- myclose ---- */
int myclose(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return FS_ERR;
    open_files[fd].used      = 0;
    open_files[fd].inode_num = -1;
    open_files[fd].offset    = 0;
    return FS_OK;
}

/* ---- myread ---- */
int myread(int fd, char *buffer, int nombre) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return FS_ERR;
    if (open_files[fd].mode == MODE_WRITE) return FS_ERR;

    int inode_num = open_files[fd].inode_num;
    Inode *inode  = get_inode(inode_num);
    if (!inode) return FS_ERR;

    int offset    = open_files[fd].offset;
    int file_size = inode->size;
    if (offset >= file_size) return 0;
    if (offset + nombre > file_size) nombre = file_size - offset;

    int bytes_read = 0;
    while (bytes_read < nombre) {
        int cur    = offset + bytes_read;
        int bidx   = cur / BLOCK_SIZE;
        int boff   = cur % BLOCK_SIZE;
        if (bidx >= MAX_BLOCKS_PER_FILE) break;
        int bnum   = inode->blocks[bidx];
        if (bnum < 0) break;
        int can    = BLOCK_SIZE - boff;
        int rem    = nombre - bytes_read;
        if (can > rem) can = rem;
        memcpy(buffer + bytes_read, fs.blocks[bnum].data + boff, can);
        bytes_read += can;
    }
    open_files[fd].offset += bytes_read;
    return bytes_read;
}

/* ---- mywrite ---- */
int mywrite(int fd, const char *buffer, int nombre) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return FS_ERR;
    if (open_files[fd].mode == MODE_READ) return FS_ERR;

    int inode_num = open_files[fd].inode_num;
    Inode *inode  = get_inode(inode_num);
    if (!inode) return FS_ERR;
    if (!(inode->permissions & PERM_W)) {
        fprintf(stderr, "mywrite: permission refusee\n");
        return FS_ERR;
    }

    int offset = open_files[fd].offset;
    int bytes_written = 0;

    while (bytes_written < nombre) {
        int cur  = offset + bytes_written;
        int bidx = cur / BLOCK_SIZE;
        int boff = cur % BLOCK_SIZE;
        if (bidx >= MAX_BLOCKS_PER_FILE) {
            fprintf(stderr, "mywrite: taille max atteinte\n"); break;
        }
        if (inode->blocks[bidx] < 0) {
            int nb = alloc_block();
            if (nb < 0) { fprintf(stderr, "mywrite: disque plein\n"); break; }
            inode->blocks[bidx] = nb;
        }
        int bnum = inode->blocks[bidx];
        int can  = BLOCK_SIZE - boff;
        int rem  = nombre - bytes_written;
        if (can > rem) can = rem;
        memcpy(fs.blocks[bnum].data + boff, buffer + bytes_written, can);
        bytes_written += can;
    }
    open_files[fd].offset += bytes_written;
    if (open_files[fd].offset > inode->size)
        inode->size = open_files[fd].offset;
    inode->mtime = time(NULL);
    return bytes_written;
}

/* ---- read/write par inode (usage interne) ---- */
int read_file_by_inode(int inode_num, char *buf, int buf_size) {
    Inode *inode = get_inode(inode_num);
    if (!inode || !inode->used || inode->type != TYPE_FILE) return FS_ERR;
    int to_read = inode->size;
    if (to_read > buf_size - 1) to_read = buf_size - 1;
    int bytes_read = 0;
    for (int j = 0; j < MAX_BLOCKS_PER_FILE && bytes_read < to_read; j++) {
        if (inode->blocks[j] < 0) break;
        int chunk = to_read - bytes_read;
        if (chunk > BLOCK_SIZE) chunk = BLOCK_SIZE;
        memcpy(buf + bytes_read, fs.blocks[inode->blocks[j]].data, chunk);
        bytes_read += chunk;
    }
    buf[bytes_read] = '\0';
    return bytes_read;
}

int write_file_by_inode(int inode_num, const char *buf, int len) {
    Inode *inode = get_inode(inode_num);
    if (!inode || !inode->used || inode->type != TYPE_FILE) return FS_ERR;
    free_inode_blocks(inode_num);
    int bytes_written = 0, bidx = 0;
    while (bytes_written < len && bidx < MAX_BLOCKS_PER_FILE) {
        int nb = alloc_block();
        if (nb < 0) break;
        inode->blocks[bidx] = nb;
        int can = BLOCK_SIZE;
        int rem = len - bytes_written;
        if (can > rem) can = rem;
        memcpy(fs.blocks[nb].data, buf + bytes_written, can);
        bytes_written += can;
        bidx++;
    }
    inode->size  = bytes_written;
    inode->mtime = time(NULL);
    return bytes_written;
}
