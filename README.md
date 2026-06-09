# Mini SGF — Système de Gestion de Fichiers

Projet réalisé dans le cadre du module **Systèmes d'exploitation**  
ISTY — IATIC3 — Année universitaire 2025/2026

---

## Équipe

| Membre | Rôle |
|---|---|
| Habib Znaidi | Shell & parsing des commandes |
| Adam Fakhr | Structures de données |
| Rayen Hmida | Gestion des fichiers |
| Oussama Aouane | Gestion des répertoires |
| Mehdi Zerdi | Sauvegarde, chargement & tests |

---

## Description

Mini SGF est un système de gestion de fichiers simulé en mémoire, développé en langage C.  
Il reproduit le fonctionnement interne d'un système de fichiers de type Unix/Linux avec :

- Un superbloc, une table d'inodes et une table de blocs
- Une arborescence de répertoires hiérarchique
- Un shell interactif pour interagir avec le système
- Une persistance des données via un fichier binaire `disk.img`

---

## Compilation

```bash
make
```

---

## Lancement

```bash
./mysgf
```

Au premier lancement, un système vierge est initialisé.  
Aux lancements suivants, le fichier `disk.img` est rechargé automatiquement.

---

## Commandes disponibles

| Commande | Description |
|---|---|
| `ls` | Lister le contenu du répertoire courant |
| `cd <rep>` | Changer de répertoire |
| `pwd` | Afficher le chemin courant |
| `mkdir <nom>` | Créer un répertoire |
| `rmdir <nom>` | Supprimer un répertoire vide |
| `echo texte > fichier` | Écrire dans un fichier |
| `cat <fichier>` | Afficher le contenu d'un fichier |
| `rm <fichier>` | Supprimer un fichier |
| `df` | Afficher les statistiques du disque |
| `help` | Afficher la liste des commandes |
| `exit` | Quitter et sauvegarder |

---

## Exemple d'utilisation

```
[sgf /] $ mkdir test
[sgf /] $ cd test
[sgf /test] $ echo hello > file.txt
[sgf /test] $ cat file.txt
hello
[sgf /test] $ df
[sgf /test] $ cd ..
[sgf /] $ rmdir test
```

---

## Structure du projet

```
/include
    fs.h
    file.h
    dir.h
    shell.h
    save.h
/src
    main.c
    fs.c
    file.c
    dir.c
    shell.c
    save.c
Makefile
README.md
```

---

## Caractéristiques techniques

- Langage : C
- 64 inodes, 256 blocs de 256 octets chacun
- Espace total simulé : 65 536 octets (64 Ko)
- Sauvegarde binaire dans `disk.img`
- Architecture modulaire (un fichier par module)
