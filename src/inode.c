/*
 * inode.c — Gestion des inodes
 */

#include "../include/fs.h"
#include "../include/inode.h"

int alloc_inode(void) {
    for (int i = 0; i < MAX_INODES; i++) {
        if (fs.inode_bitmap[i] == 0) {
            fs.inode_bitmap[i] = 1;
            fs.sb.free_inodes--;
            memset(&fs.inodes[i], 0, sizeof(Inode));
            for (int j = 0; j < MAX_BLOCKS_PER_FILE; j++)
                fs.inodes[i].blocks[j] = -1;
            fs.inodes[i].used  = 1;
            fs.inodes[i].mtime = time(NULL);
            return i;
        }
    }
    fprintf(stderr, "ERREUR : aucun inode libre\n");
    return FS_ERR;
}

void free_inode(int n) {
    if (n < 0 || n >= MAX_INODES) return;
    memset(&fs.inodes[n], 0, sizeof(Inode));
    for (int j = 0; j < MAX_BLOCKS_PER_FILE; j++)
        fs.inodes[n].blocks[j] = -1;
    fs.inode_bitmap[n] = 0;
    fs.sb.free_inodes++;
}

Inode *get_inode(int n) {
    if (n < 0 || n >= MAX_INODES) return NULL;
    return &fs.inodes[n];
}

void init_inode(int n, int type, int permissions) {
    Inode *inode = get_inode(n);
    if (!inode) return;
    inode->type        = type;
    inode->permissions = permissions;
    inode->size        = 0;
    inode->nlinks      = 1;
    inode->mtime       = time(NULL);
}

void permissions_to_str(int type, int perms, char *out) {
    out[0] = (type == TYPE_DIR) ? 'd' : '-';
    out[1] = (perms & PERM_R) ? 'r' : '-';
    out[2] = (perms & PERM_W) ? 'w' : '-';
    out[3] = (perms & PERM_X) ? 'x' : '-';
    out[4] = out[1]; out[5] = out[2]; out[6] = out[3];
    out[7] = out[1]; out[8] = out[2]; out[9] = out[3];
    out[10] = '\0';
}
