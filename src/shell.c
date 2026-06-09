/*
 * shell.c — Mini interpréteur de commandes
 *
 * CORRECTION : le prompt affiche désormais le chemin courant.
 *   Exemple : [sgf /habib] $
 *
 * Commandes : ls  mkdir  rmdir  cd  pwd  cat  cp  rm  mv  ln
 *             echo [texte] > fic    df    save    help    exit
 */

#include "../include/fs.h"
#include "../include/shell.h"
#include "../include/dir_ops.h"
#include "../include/file_ops.h"
#include "../include/inode.h"
#include "../include/block.h"
#include "../include/save.h"

/* ── Couleurs ANSI ─────────────────────────────── */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_RED     "\033[1;31m"
#define C_GREEN   "\033[1;32m"
#define C_YELLOW  "\033[1;33m"
#define C_BLUE    "\033[1;34m"
#define C_CYAN    "\033[1;36m"
#define C_GRAY    "\033[0;37m"

/* ── Affiche le prompt dynamique ───────────────── */
static void print_prompt(void) {
    /* Format : [sgf <chemin>] $
     * Couleurs : crochet gris, "sgf" cyan, chemin jaune, $ vert */
    printf(C_GRAY "[" C_CYAN "sgf " C_YELLOW "%s" C_GRAY "]"
           C_GREEN " $ " C_RESET, fs.current_path);
    fflush(stdout);
}

/* ── Bannière de démarrage ─────────────────────── */
static void print_banner(void) {
    printf("\n");
    printf(C_CYAN "  ╔═══════════════════════════════════════╗\n");
    printf(       "  ║" C_YELLOW "   Mini SGF  —  ISTY IATIC3 2025/26  " C_CYAN "║\n");
    printf(       "  ║" C_GRAY   "     Systeme de Gestion de Fichiers  " C_CYAN "║\n");
    printf(       "  ║" C_GREEN  "     'help' pour la liste de cmds    " C_CYAN "║\n");
    printf(       "  ╚═══════════════════════════════════════╝\n" C_RESET);
    printf("\n");
}

/* ── run_shell ─────────────────────────────────── */
void run_shell(void) {
    char line[MAX_CMD_LEN];
    print_banner();

    while (1) {
        print_prompt();
        if (!fgets(line, MAX_CMD_LEN, stdin)) {
            printf("\n"); break;
        }
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (strlen(line) == 0) continue;
        execute_command(line);
    }
    save_fs(DISK_IMAGE);
}

/* ── Parseur de ligne ──────────────────────────── */
static int parse_line(char *line, char **argv, int max_args, char **redir) {
    int argc = 0;
    *redir   = NULL;
    char *p  = line;

    while (*p && argc < max_args - 1) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        if (*p == '>') {                        /* redirection */
            p++;
            while (*p == ' ' || *p == '\t') p++;
            if (*p) {
                *redir = p;
                char *e = p;
                while (*e && *e != ' ' && *e != '\t') e++;
                *e = '\0';
            }
            break;
        }

        if (*p == '"' || *p == '\'') {          /* guillemets */
            char q = *p++;
            argv[argc++] = p;
            while (*p && *p != q) p++;
            if (*p) *p++ = '\0';
        } else {                                /* mot normal */
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '>') p++;
            if (*p == '>') { *p = '\0'; }
            else if (*p)   { *p++ = '\0'; }
        }
    }
    argv[argc] = NULL;
    return argc;
}

/* ── execute_command ───────────────────────────── */
void execute_command(char *line) {
    char *argv[MAX_ARGS];
    char *redir = NULL;
    int argc = parse_line(line, argv, MAX_ARGS, &redir);
    if (argc == 0) return;

    char *cmd = argv[0];

    if (!strcmp(cmd,"exit") || !strcmp(cmd,"quit")) {
        save_fs(DISK_IMAGE);
        printf(C_GREEN "  Au revoir !\n" C_RESET);
        exit(0);
    }
    else if (!strcmp(cmd,"ls"))    cmd_ls(argc, argv);
    else if (!strcmp(cmd,"mkdir")) cmd_mkdir(argc, argv);
    else if (!strcmp(cmd,"rmdir")) cmd_rmdir(argc, argv);
    else if (!strcmp(cmd,"cd"))    cmd_cd(argc, argv);
    else if (!strcmp(cmd,"pwd"))   cmd_pwd();
    else if (!strcmp(cmd,"cat"))   cmd_cat(argc, argv);
    else if (!strcmp(cmd,"cp"))    cmd_cp(argc, argv);
    else if (!strcmp(cmd,"rm"))    cmd_rm(argc, argv);
    else if (!strcmp(cmd,"mv"))    cmd_mv(argc, argv);
    else if (!strcmp(cmd,"ln"))    cmd_ln(argc, argv);
    else if (!strcmp(cmd,"echo"))  cmd_echo(argc, argv, redir);
    else if (!strcmp(cmd,"df"))    cmd_df();
    else if (!strcmp(cmd,"save"))  save_fs(DISK_IMAGE);
    else if (!strcmp(cmd,"help"))  cmd_help();
    else
        fprintf(stderr, C_RED "  Erreur" C_RESET ": '%s' commande introuvable"
                " (tapez 'help')\n", cmd);
}

/* ═══════════════════════════════════════════════
 *  Implémentation des commandes
 * ═══════════════════════════════════════════════ */

/* ── ls [-l] ── */
void cmd_ls(int argc, char **argv) {
    int lf = 0;
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "-l")) lf = 1;
    fs_ls(lf);
}

/* ── mkdir ── */
void cmd_mkdir(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: mkdir <nom>\n"); return; }
    for (int i = 1; i < argc; i++)
        if (fs_mkdir(argv[i]) == FS_OK)
            printf("  Repertoire '" C_BLUE "%s" C_RESET "' cree\n", argv[i]);
}

/* ── rmdir ── */
void cmd_rmdir(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: rmdir <nom>\n"); return; }
    for (int i = 1; i < argc; i++)
        if (fs_rmdir(argv[i]) == FS_OK)
            printf("  Repertoire '%s' supprime\n", argv[i]);
}

/* ── cd ──
 * FIX : affiche le nouveau chemin après chaque cd réussi.
 */
void cmd_cd(int argc, char **argv) {
    const char *dest = (argc < 2) ? "/" : argv[1];
    if (fs_cd(dest) == FS_OK) {
        /* Rien à afficher : le prompt suivant montrera le nouveau chemin */
    }
}

/* ── pwd ── */
void cmd_pwd(void) {
    printf("  %s\n", fs.current_path);
}

/* ── cat ── */
void cmd_cat(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: cat <fichier>\n"); return; }
    char buf[MAX_BLOCKS_PER_FILE * BLOCK_SIZE + 1];
    for (int i = 1; i < argc; i++) {
        int inode_num;
        if (strchr(argv[i], '/')) {
            int par; char comp[MAX_FILENAME];
            split_path(argv[i], &par, comp);
            inode_num = dir_find_entry(par, comp);
        } else {
            inode_num = dir_find_entry(fs.current_dir, argv[i]);
        }
        if (inode_num < 0) { fprintf(stderr, "  cat: '%s' introuvable\n", argv[i]); continue; }
        Inode *in = get_inode(inode_num);
        if (in->type == TYPE_DIR) { fprintf(stderr, "  cat: '%s' est un repertoire\n", argv[i]); continue; }
        int n = read_file_by_inode(inode_num, buf, sizeof(buf));
        if (n > 0) {
            fwrite(buf, 1, n, stdout);
            if (buf[n-1] != '\n') printf("\n");
        }
    }
}

/* ── cp ── */
void cmd_cp(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "  Usage: cp <src> <dst>\n"); return; }

    /* Résoudre la source */
    int src_inode = resolve_path(argv[1]);
    if (src_inode < 0) src_inode = dir_find_entry(fs.current_dir, argv[1]);
    if (src_inode < 0) { fprintf(stderr, "  cp: '%s' introuvable\n", argv[1]); return; }

    Inode *si = get_inode(src_inode);
    if (si->type == TYPE_DIR) { fprintf(stderr, "  cp: copie de repertoires non supportee\n"); return; }

    char buf[MAX_BLOCKS_PER_FILE * BLOCK_SIZE + 1];
    int n = read_file_by_inode(src_inode, buf, sizeof(buf));
    if (n < 0) { fprintf(stderr, "  cp: erreur de lecture\n"); return; }

    /* Résoudre la destination */
    int dst_parent = fs.current_dir;
    char dst_name[MAX_FILENAME];
    if (strchr(argv[2], '/'))
        split_path(argv[2], &dst_parent, dst_name);
    else {
        strncpy(dst_name, argv[2], MAX_FILENAME - 1);
        dst_name[MAX_FILENAME - 1] = '\0';
    }

    /* Si la destination est un répertoire, copier dedans */
    int dck = dir_find_entry(dst_parent, dst_name);
    if (dck >= 0 && get_inode(dck)->type == TYPE_DIR) {
        dst_parent = dck;
        char *sl = strrchr(argv[1], '/');
        strncpy(dst_name, sl ? sl + 1 : argv[1], MAX_FILENAME - 1);
        dst_name[MAX_FILENAME - 1] = '\0';
        dck = dir_find_entry(dst_parent, dst_name);
    }

    /* Créer ou écraser la destination */
    int saved_dir = fs.current_dir;
    char saved_path[MAX_PATH];
    strncpy(saved_path, fs.current_path, MAX_PATH - 1);
    fs.current_dir = dst_parent;

    int dst_inode = (dck >= 0) ? dck : mycreat(dst_name, si->permissions);
    fs.current_dir = saved_dir;
    strncpy(fs.current_path, saved_path, MAX_PATH - 1);

    if (dst_inode < 0) { fprintf(stderr, "  cp: impossible de creer '%s'\n", dst_name); return; }
    if (write_file_by_inode(dst_inode, buf, n) < 0)
        fprintf(stderr, "  cp: erreur d'ecriture\n");
}

/* ── rm ── */
void cmd_rm(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: rm <fichier> ...\n"); return; }
    for (int i = 1; i < argc; i++)
        if (fs_unlink(argv[i]) == FS_OK)
            printf("  '%s' supprime\n", argv[i]);
}

/* ── mv ── */
void cmd_mv(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "  Usage: mv <src> <dst>\n"); return; }
    int inode_num = dir_find_entry(fs.current_dir, argv[1]);
    if (inode_num < 0) { fprintf(stderr, "  mv: '%s' introuvable\n", argv[1]); return; }
    if (dir_find_entry(fs.current_dir, argv[2]) >= 0) { fprintf(stderr, "  mv: '%s' existe deja\n", argv[2]); return; }
    if (dir_add_entry(fs.current_dir, argv[2], inode_num) < 0) return;
    dir_remove_entry(fs.current_dir, argv[1]);
    fs.inodes[inode_num].mtime = time(NULL);
    printf("  '%s' renomme en '%s'\n", argv[1], argv[2]);
}

/* ── ln ── */
void cmd_ln(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "  Usage: ln <src> <lien>\n"); return; }
    if (fs_link(argv[1], argv[2]) == FS_OK)
        printf("  Lien '%s' -> '%s' cree\n", argv[2], argv[1]);
}

/* ── echo [texte] [> fichier] ── */
void cmd_echo(int argc, char **argv, const char *redir) {
    char buf[MAX_BLOCKS_PER_FILE * BLOCK_SIZE];
    buf[0] = '\0';
    for (int i = 1; i < argc; i++) {
        if (i > 1) strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
        strncat(buf, argv[i], sizeof(buf) - strlen(buf) - 1);
    }
    strncat(buf, "\n", sizeof(buf) - strlen(buf) - 1);

    if (redir) {
        int inode_num = dir_find_entry(fs.current_dir, redir);
        if (inode_num < 0) inode_num = mycreat(redir, PERM_RW);
        if (inode_num < 0) { fprintf(stderr, "  echo: impossible de creer '%s'\n", redir); return; }
        write_file_by_inode(inode_num, buf, strlen(buf));
    } else {
        printf("  %s", buf);
    }
}

/* ── df ── */
void cmd_df(void) { fs_print_info(); }

/* ── help ── */
void cmd_help(void) {
    printf("\n");
    printf(C_CYAN "  ╔══════════════════════════════════════════════════╗\n");
    printf(       "  ║" C_YELLOW "              Commandes disponibles               " C_CYAN "║\n");
    printf(       "  ╠══════════════╦═══════════════════════════════════╣\n");
    printf(       "  ║" C_GREEN " ls [-l]      " C_CYAN "║" C_RESET " Liste le repertoire courant       " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " mkdir <nom>  " C_CYAN "║" C_RESET " Creer un repertoire               " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " rmdir <nom>  " C_CYAN "║" C_RESET " Supprimer un repertoire vide      " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " cd <chemin>  " C_CYAN "║" C_RESET " Changer de repertoire             " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " pwd          " C_CYAN "║" C_RESET " Afficher le chemin courant        " C_CYAN "║\n");
    printf(       "  ╠══════════════╬═══════════════════════════════════╣\n");
    printf(       "  ║" C_GREEN " cat <fic>    " C_CYAN "║" C_RESET " Afficher le contenu d'un fichier  " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " cp <s> <d>   " C_CYAN "║" C_RESET " Copier un fichier                 " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " rm <fic>     " C_CYAN "║" C_RESET " Supprimer un fichier              " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " mv <s> <d>   " C_CYAN "║" C_RESET " Renommer/deplacer un fichier      " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " ln <s> <l>   " C_CYAN "║" C_RESET " Creer un lien dur                 " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " echo t > f   " C_CYAN "║" C_RESET " Ecrire du texte dans un fichier   " C_CYAN "║\n");
    printf(       "  ╠══════════════╬═══════════════════════════════════╣\n");
    printf(       "  ║" C_GREEN " df           " C_CYAN "║" C_RESET " Infos superbloc (espace, inodes)  " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " save         " C_CYAN "║" C_RESET " Sauvegarder le SGF                " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " help         " C_CYAN "║" C_RESET " Afficher cette aide               " C_CYAN "║\n");
    printf(       "  ║" C_GREEN " exit/quit    " C_CYAN "║" C_RESET " Quitter (sauvegarde auto)         " C_CYAN "║\n");
    printf(       "  ╚══════════════╩═══════════════════════════════════╝\n" C_RESET);
    printf("\n");
}
