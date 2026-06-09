/*
 * dir_ops.c — Primitives répertoires, navigation, listage
 */

#include "../include/fs.h"
#include "../include/dir_ops.h"
#include "../include/inode.h"
#include "../include/block.h"
#include "../include/file_ops.h"

/* ---- Ajout d'une entrée dans un répertoire ---- */
int dir_add_entry(int dir_inode, const char *name, int inode_num) {
    Inode *dir = get_inode(dir_inode);
    if (!dir || dir->type != TYPE_DIR) return FS_ERR;
    if (strlen(name) >= MAX_FILENAME) return FS_ERR;

    for (int b = 0; b < MAX_BLOCKS_PER_FILE; b++) {
        int bnum;
        if (dir->blocks[b] < 0) {
            bnum = alloc_block();
            if (bnum < 0) return FS_ERR;
            dir->blocks[b] = bnum;
            DirEntry *ent = (DirEntry *) fs.blocks[bnum].data;
            for (int k = 0; k < ENTRIES_PER_BLOCK; k++) {
                memset(ent[k].name, 0, MAX_FILENAME);
                ent[k].inode_num = -1;
            }
        } else {
            bnum = dir->blocks[b];
        }
        DirEntry *entries = (DirEntry *) fs.blocks[bnum].data;
        for (int e = 0; e < ENTRIES_PER_BLOCK; e++) {
            if (entries[e].inode_num == -1) {
                strncpy(entries[e].name, name, MAX_FILENAME - 1);
                entries[e].name[MAX_FILENAME - 1] = '\0';
                entries[e].inode_num = inode_num;
                dir->size += DIR_ENTRY_SIZE;
                dir->mtime = time(NULL);
                return FS_OK;
            }
        }
    }
    return FS_ERR;
}

/* ---- Recherche d'une entrée ---- */
int dir_find_entry(int dir_inode, const char *name) {
    Inode *dir = get_inode(dir_inode);
    if (!dir || dir->type != TYPE_DIR) return FS_ERR;
    for (int b = 0; b < MAX_BLOCKS_PER_FILE; b++) {
        if (dir->blocks[b] < 0) continue;
        DirEntry *entries = (DirEntry *) fs.blocks[dir->blocks[b]].data;
        for (int e = 0; e < ENTRIES_PER_BLOCK; e++) {
            if (entries[e].inode_num != -1 &&
                strncmp(entries[e].name, name, MAX_FILENAME) == 0)
                return entries[e].inode_num;
        }
    }
    return FS_ERR;
}

/* ---- Suppression d'une entrée ---- */
int dir_remove_entry(int dir_inode, const char *name) {
    Inode *dir = get_inode(dir_inode);
    if (!dir || dir->type != TYPE_DIR) return FS_ERR;
    for (int b = 0; b < MAX_BLOCKS_PER_FILE; b++) {
        if (dir->blocks[b] < 0) continue;
        DirEntry *entries = (DirEntry *) fs.blocks[dir->blocks[b]].data;
        for (int e = 0; e < ENTRIES_PER_BLOCK; e++) {
            if (entries[e].inode_num != -1 &&
                strncmp(entries[e].name, name, MAX_FILENAME) == 0) {
                memset(entries[e].name, 0, MAX_FILENAME);
                entries[e].inode_num = -1;
                if (dir->size >= DIR_ENTRY_SIZE) dir->size -= DIR_ENTRY_SIZE;
                dir->mtime = time(NULL);
                return FS_OK;
            }
        }
    }
    return FS_ERR;
}

/* ---- Comptage des entrées (hors . et ..) ---- */
int dir_count_entries(int dir_inode) {
    Inode *dir = get_inode(dir_inode);
    if (!dir || dir->type != TYPE_DIR) return FS_ERR;
    int count = 0;
    for (int b = 0; b < MAX_BLOCKS_PER_FILE; b++) {
        if (dir->blocks[b] < 0) continue;
        DirEntry *entries = (DirEntry *) fs.blocks[dir->blocks[b]].data;
        for (int e = 0; e < ENTRIES_PER_BLOCK; e++) {
            if (entries[e].inode_num != -1 &&
                strcmp(entries[e].name, ".") != 0 &&
                strcmp(entries[e].name, "..") != 0)
                count++;
        }
    }
    return count;
}

/* ---- Résolution de chemin (absolu ou relatif) ---- */
int resolve_path(const char *path) {
    if (!path || strlen(path) == 0) return FS_ERR;
    if (strcmp(path, "/") == 0) return ROOT_INODE;

    int cur = (path[0] == '/') ? ROOT_INODE : fs.current_dir;

    char tmp[MAX_PATH];
    strncpy(tmp, path, MAX_PATH - 1);
    tmp[MAX_PATH - 1] = '\0';

    char *token = strtok(tmp, "/");
    while (token) {
        if (strlen(token) == 0) { token = strtok(NULL, "/"); continue; }
        int next = dir_find_entry(cur, token);
        if (next < 0) return FS_ERR;
        cur = next;
        token = strtok(NULL, "/");
    }
    return cur;
}

/* ---- Découpe d'un chemin en (parent, dernier composant) ---- */
int split_path(const char *path, int *parent_inode, char *last_comp) {
    if (!path || strlen(path) == 0) return FS_ERR;
    char tmp[MAX_PATH];
    strncpy(tmp, path, MAX_PATH - 1);
    tmp[MAX_PATH - 1] = '\0';
    char *slash = strrchr(tmp, '/');
    if (!slash) {
        *parent_inode = fs.current_dir;
        strncpy(last_comp, tmp, MAX_FILENAME - 1);
        last_comp[MAX_FILENAME - 1] = '\0';
        return FS_OK;
    }
    if (slash == tmp) {
        *parent_inode = ROOT_INODE;
    } else {
        *slash = '\0';
        *parent_inode = resolve_path(tmp);
        if (*parent_inode < 0) return FS_ERR;
    }
    strncpy(last_comp, slash + 1, MAX_FILENAME - 1);
    last_comp[MAX_FILENAME - 1] = '\0';
    return FS_OK;
}

/* ---- mkdir ---- */
int fs_mkdir(const char *name) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_FILENAME) {
        fprintf(stderr, "mkdir: nom invalide\n"); return FS_ERR;
    }
    if (dir_find_entry(fs.current_dir, name) >= 0) {
        fprintf(stderr, "mkdir: '%s' existe deja\n", name); return FS_ERR;
    }
    int inode_num = alloc_inode();
    if (inode_num < 0) return FS_ERR;

    init_inode(inode_num, TYPE_DIR, PERM_RWX);
    fs.inodes[inode_num].nlinks = 2;

    int bnum = alloc_block();
    if (bnum < 0) { free_inode(inode_num); return FS_ERR; }
    fs.inodes[inode_num].blocks[0] = bnum;

    DirEntry *entries = (DirEntry *) fs.blocks[bnum].data;
    for (int i = 0; i < ENTRIES_PER_BLOCK; i++) {
        memset(entries[i].name, 0, MAX_FILENAME);
        entries[i].inode_num = -1;
    }
    strncpy(entries[0].name, ".", MAX_FILENAME - 1);
    entries[0].inode_num = inode_num;
    strncpy(entries[1].name, "..", MAX_FILENAME - 1);
    entries[1].inode_num = fs.current_dir;
    fs.inodes[inode_num].size = 2 * DIR_ENTRY_SIZE;

    if (dir_add_entry(fs.current_dir, name, inode_num) < 0) {
        free_inode_blocks(inode_num); free_inode(inode_num); return FS_ERR;
    }
    fs.inodes[fs.current_dir].nlinks++;
    return FS_OK;
}

/* ---- rmdir ---- */
int fs_rmdir(const char *name) {
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        fprintf(stderr, "rmdir: impossible de supprimer '.' ou '..'\n"); return FS_ERR;
    }
    int inode_num = dir_find_entry(fs.current_dir, name);
    if (inode_num < 0) { fprintf(stderr, "rmdir: '%s' introuvable\n", name); return FS_ERR; }
    Inode *inode = get_inode(inode_num);
    if (!inode || inode->type != TYPE_DIR) {
        fprintf(stderr, "rmdir: '%s' n'est pas un repertoire\n", name); return FS_ERR;
    }
    if (dir_count_entries(inode_num) > 0) {
        fprintf(stderr, "rmdir: '%s' n'est pas vide\n", name); return FS_ERR;
    }
    dir_remove_entry(fs.current_dir, name);
    fs.inodes[fs.current_dir].nlinks--;
    free_inode_blocks(inode_num);
    free_inode(inode_num);
    return FS_OK;
}

/* ---- link ---- */
int fs_link(const char *name1, const char *name2) {
    int inode_num = dir_find_entry(fs.current_dir, name1);
    if (inode_num < 0) { fprintf(stderr, "ln: '%s' introuvable\n", name1); return FS_ERR; }
    Inode *inode = get_inode(inode_num);
    if (inode->type == TYPE_DIR) { fprintf(stderr, "ln: lien dur sur repertoire interdit\n"); return FS_ERR; }
    if (dir_find_entry(fs.current_dir, name2) >= 0) { fprintf(stderr, "ln: '%s' existe deja\n", name2); return FS_ERR; }
    if (dir_add_entry(fs.current_dir, name2, inode_num) < 0) return FS_ERR;
    inode->nlinks++;
    return FS_OK;
}

/* ---- unlink ---- */
int fs_unlink(const char *name) {
    int inode_num = dir_find_entry(fs.current_dir, name);
    if (inode_num < 0) { fprintf(stderr, "rm: '%s' introuvable\n", name); return FS_ERR; }
    Inode *inode = get_inode(inode_num);
    if (inode->type == TYPE_DIR) { fprintf(stderr, "rm: '%s' est un repertoire, utilisez rmdir\n", name); return FS_ERR; }
    dir_remove_entry(fs.current_dir, name);
    inode->nlinks--;
    if (inode->nlinks <= 0) { free_inode_blocks(inode_num); free_inode(inode_num); }
    return FS_OK;
}

/* ---- cd : correction principale ----
 * FIX : reconstruit current_path proprement depuis resolve_path
 * au lieu de le calculer à la main (ce qui causait des bugs).
 */
int fs_cd(const char *path) {
    int inode_num = resolve_path(path);
    if (inode_num < 0) { fprintf(stderr, "cd: '%s' introuvable\n", path); return FS_ERR; }
    Inode *inode = get_inode(inode_num);
    if (!inode || inode->type != TYPE_DIR) { fprintf(stderr, "cd: '%s' n'est pas un repertoire\n", path); return FS_ERR; }

    fs.current_dir = inode_num;

    /* Reconstruire le chemin absolu proprement */
    if (path[0] == '/') {
        /* Chemin absolu fourni → l'utiliser directement (nettoyé) */
        strncpy(fs.current_path, path, MAX_PATH - 1);
        fs.current_path[MAX_PATH - 1] = '\0';
        /* Supprimer le slash final sauf si racine */
        int len = strlen(fs.current_path);
        if (len > 1 && fs.current_path[len - 1] == '/')
            fs.current_path[len - 1] = '\0';
    } else if (strcmp(path, "..") == 0) {
        /* Monter d'un niveau : trouver le dernier '/' */
        char *last = strrchr(fs.current_path, '/');
        if (last && last != fs.current_path)
            *last = '\0';       /* ex: /rep1/rep2 → /rep1 */
        else
            strncpy(fs.current_path, "/", MAX_PATH - 1);
    } else if (strcmp(path, ".") == 0) {
        /* Rester ici : ne rien changer */
    } else {
        /* Chemin relatif simple (un seul composant sans '/') */
        if (strcmp(fs.current_path, "/") != 0)
            strncat(fs.current_path, "/", MAX_PATH - strlen(fs.current_path) - 1);
        strncat(fs.current_path, path, MAX_PATH - strlen(fs.current_path) - 1);
    }

    return FS_OK;
}

/* ---- ls ---- */
void fs_ls(int long_format) { fs_ls_dir(fs.current_dir, long_format); }

void fs_ls_dir(int dir_inode, int long_format) {
    Inode *dir = get_inode(dir_inode);
    if (!dir || dir->type != TYPE_DIR) { fprintf(stderr, "ls: pas un repertoire\n"); return; }

    if (long_format) printf("total %d\n", fs.sb.total_blocks - fs.sb.free_blocks);

    for (int b = 0; b < MAX_BLOCKS_PER_FILE; b++) {
        if (dir->blocks[b] < 0) continue;
        DirEntry *entries = (DirEntry *) fs.blocks[dir->blocks[b]].data;
        for (int e = 0; e < ENTRIES_PER_BLOCK; e++) {
            if (entries[e].inode_num < 0) continue;
            Inode *ei = get_inode(entries[e].inode_num);
            if (!ei) continue;

            if (long_format) {
                char pstr[12], tstr[20];
                struct tm *tm_info = localtime(&ei->mtime);
                strftime(tstr, sizeof(tstr), "%d %b %H:%M", tm_info);
                permissions_to_str(ei->type, ei->permissions, pstr);
                printf("%s. %-2d user  user  %6d %s  %s\n",
                       pstr, ei->nlinks, ei->size, tstr, entries[e].name);
            } else {
                if (ei->type == TYPE_DIR)
                    printf("\033[1;34m%s\033[0m  ", entries[e].name);
                else
                    printf("%s  ", entries[e].name);
            }
        }
    }
    if (!long_format) printf("\n");
}
