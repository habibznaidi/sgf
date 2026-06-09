/*
 * save.c — Sauvegarde et chargement binaire du SGF
 */

#include "../include/fs.h"
#include "../include/save.h"

#define FS_MAGIC 0xDEAD5AF0

typedef struct { unsigned int magic; size_t fs_size; } SaveHeader;

int save_fs(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) { fprintf(stderr, "save: impossible d'ouvrir '%s'\n", filename); return -1; }
    SaveHeader h = { FS_MAGIC, sizeof(FileSystem) };
    fwrite(&h,  sizeof(SaveHeader), 1, f);
    fwrite(&fs, sizeof(FileSystem), 1, f);
    fclose(f);
    printf("  Sauvegarde OK → '%s'\n", filename);
    return 0;
}

int load_fs(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;
    SaveHeader h;
    if (fread(&h, sizeof(SaveHeader), 1, f) != 1 ||
        h.magic != FS_MAGIC || h.fs_size != sizeof(FileSystem)) {
        fprintf(stderr, "load: fichier invalide ou incompatible\n");
        fclose(f); return -1;
    }
    fread(&fs, sizeof(FileSystem), 1, f);
    fclose(f);
    printf("  SGF charge depuis '%s'\n", filename);
    return 0;
}
