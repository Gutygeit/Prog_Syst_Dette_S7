#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_LINE 1024
#define MAX_HISTORY 10
#define AUTORIZATIONS (O_WRONLY | O_CREAT | O_APPEND)

void shell();
void handle_sigint(int sig);
void handle_sigtstp(int sig);
void handle_sigterm(int sig);
void add_to_history(char *command);
void print_history();
void clear_history();
void execute_command(char *command);
void print_help();
void exit_shell();
void change_directory(char **args);
char *get_path();


#endif
