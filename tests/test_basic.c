/*
 * test_basic.c — Tests de validation du Mini SGF
 * Compile : make test
 */
#include <stdio.h>
#include <string.h>
#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/block.h"
#include "../include/file_ops.h"
#include "../include/dir_ops.h"
#include "../include/save.h"

static int ok = 0, ko = 0;

#define TEST(desc, cond) do { \
    if (cond) { printf("  [OK] %s\n", desc); ok++; } \
    else      { printf("  [KO] %s  (ligne %d)\n", desc, __LINE__); ko++; } \
} while(0)

void test_format(void) {
    printf("\n--- Test 1 : Formatage ---\n");
    fs_format();
    TEST("total_inodes == MAX_INODES",  fs.sb.total_inodes == MAX_INODES);
    TEST("total_blocks == MAX_BLOCKS",  fs.sb.total_blocks == MAX_BLOCKS);
    TEST("inode racine utilise",        fs.inodes[ROOT_INODE].used == 1);
    TEST("inode racine = repertoire",   fs.inodes[ROOT_INODE].type == TYPE_DIR);
    TEST("current_dir = 0",            fs.current_dir == ROOT_INODE);
    TEST("current_path = '/'",         strcmp(fs.current_path, "/") == 0);
    TEST("entree '.' dans racine",     dir_find_entry(ROOT_INODE, ".") == ROOT_INODE);
    TEST("entree '..' dans racine",    dir_find_entry(ROOT_INODE, "..") == ROOT_INODE);
}

void test_fichier(void) {
    printf("\n--- Test 2 : Fichier (creat/write/read) ---\n");
    fs_format(); init_open_files();
    int in = mycreat("test.txt", PERM_RW);
    TEST("mycreat valide", in >= 0);
    TEST("trouve dans racine", dir_find_entry(ROOT_INODE, "test.txt") == in);
    int fd = myopen("test.txt", MODE_WRITE);
    TEST("myopen write valide", fd >= 0);
    const char *msg = "Bonjour SGF !";
    int w = mywrite(fd, msg, strlen(msg));
    TEST("mywrite correct", w == (int)strlen(msg));
    myclose(fd);
    TEST("taille inode correcte", fs.inodes[in].size == (int)strlen(msg));
    fd = myopen("test.txt", MODE_READ);
    char buf[256] = {0};
    int r = myread(fd, buf, sizeof(buf)-1);
    TEST("myread correct", r == (int)strlen(msg));
    TEST("contenu identique", strcmp(buf, msg) == 0);
    myclose(fd);
}

void test_repertoires(void) {
    printf("\n--- Test 3 : Repertoires ---\n");
    fs_format(); init_open_files();
    TEST("mkdir docs OK",       fs_mkdir("docs") == FS_OK);
    TEST("docs visible en ls",  dir_find_entry(ROOT_INODE, "docs") >= 0);
    TEST("cd docs OK",          fs_cd("docs") == FS_OK);
    TEST("current != racine",   fs.current_dir != ROOT_INODE);
    /* FIX : vérifier que le chemin est mis à jour */
    TEST("current_path = /docs", strcmp(fs.current_path, "/docs") == 0);
    int f = mycreat("note.txt", PERM_RW);
    TEST("creat dans sous-rep", f >= 0);
    TEST("cd .. OK",            fs_cd("..") == FS_OK);
    TEST("retour racine",       fs.current_dir == ROOT_INODE);
    TEST("current_path = /",   strcmp(fs.current_path, "/") == 0);
    TEST("rmdir non vide echoue", fs_rmdir("docs") == FS_ERR);
    fs_cd("docs"); fs_unlink("note.txt"); fs_cd("..");
    TEST("rmdir vide OK",       fs_rmdir("docs") == FS_OK);
    TEST("docs absent",         dir_find_entry(ROOT_INODE, "docs") < 0);
}

void test_liens(void) {
    printf("\n--- Test 4 : Liens durs ---\n");
    fs_format(); init_open_files();
    int in = mycreat("orig.txt", PERM_RW);
    write_file_by_inode(in, "data", 4);
    TEST("link OK",             fs_link("orig.txt", "lien.txt") == FS_OK);
    TEST("lien -> meme inode",  dir_find_entry(ROOT_INODE, "lien.txt") == in);
    TEST("nlinks == 2",         fs.inodes[in].nlinks == 2);
    TEST("unlink orig OK",      fs_unlink("orig.txt") == FS_OK);
    TEST("nlinks == 1",         fs.inodes[in].nlinks == 1);
    TEST("inode encore vivant", fs.inodes[in].used == 1);
    char b[64]; read_file_by_inode(in, b, sizeof(b));
    TEST("lecture via lien OK", strncmp(b, "data", 4) == 0);
    TEST("unlink lien OK",      fs_unlink("lien.txt") == FS_OK);
    TEST("inode libere",        fs.inodes[in].used == 0);
}

void test_sauvegarde(void) {
    printf("\n--- Test 5 : Sauvegarde / Rechargement ---\n");
    fs_format(); init_open_files();
    fs_mkdir("projets");
    fs_cd("projets");
    int in = mycreat("readme.txt", PERM_RW);
    write_file_by_inode(in, "ISTY SGF", 8);
    fs_cd("..");
    TEST("save OK",             save_fs("_test.img") == 0);
    fs_format();
    TEST("apres format absent", dir_find_entry(ROOT_INODE, "projets") < 0);
    TEST("load OK",             load_fs("_test.img") == 0);
    int pi = dir_find_entry(ROOT_INODE, "projets");
    TEST("projets retrouve",    pi >= 0);
    int ri = dir_find_entry(pi, "readme.txt");
    TEST("readme retrouve",     ri >= 0);
    char b[64]; read_file_by_inode(ri, b, sizeof(b));
    TEST("contenu correct",     strncmp(b, "ISTY SGF", 8) == 0);
    remove("_test.img");
}

void test_chemins(void) {
    printf("\n--- Test 6 : Resolution de chemins ---\n");
    fs_format(); init_open_files();
    fs_mkdir("rep1");
    int r1 = dir_find_entry(ROOT_INODE, "rep1");
    fs_cd("rep1"); fs_mkdir("rep2");
    int r2 = dir_find_entry(r1, "rep2");
    fs_cd("rep2"); mycreat("fic.txt", PERM_RW);
    int fi = dir_find_entry(r2, "fic.txt");
    fs_cd("/");
    TEST("resolve /rep1",          resolve_path("/rep1") == r1);
    TEST("resolve /rep1/rep2",     resolve_path("/rep1/rep2") == r2);
    TEST("resolve /rep1/rep2/fic", resolve_path("/rep1/rep2/fic.txt") == fi);
    fs_cd("rep1");
    TEST("resolve relatif rep2",   resolve_path("rep2") == r2);
}

int main(void) {
    printf("=========================================\n");
    printf("  Tests Mini SGF — ISTY IATIC3\n");
    printf("=========================================\n");
    test_format();
    test_fichier();
    test_repertoires();
    test_liens();
    test_sauvegarde();
    test_chemins();
    printf("\n=========================================\n");
    printf("  Resultats : %d OK  /  %d KO\n", ok, ko);
    printf("=========================================\n");
    return (ko == 0) ? 0 : 1;
}
