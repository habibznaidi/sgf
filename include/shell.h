#ifndef SHELL_H
#define SHELL_H

#define MAX_ARGS    16
#define MAX_CMD_LEN 512

void run_shell(void);
void execute_command(char *line);

void cmd_ls(int argc, char **argv);
void cmd_mkdir(int argc, char **argv);
void cmd_rmdir(int argc, char **argv);
void cmd_cd(int argc, char **argv);
void cmd_cat(int argc, char **argv);
void cmd_cp(int argc, char **argv);
void cmd_rm(int argc, char **argv);
void cmd_mv(int argc, char **argv);
void cmd_ln(int argc, char **argv);
void cmd_echo(int argc, char **argv, const char *redirect_file);
void cmd_df(void);
void cmd_pwd(void);
void cmd_help(void);

#endif
