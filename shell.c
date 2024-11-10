#include "shell.h"

// History of commands entered in the shell
char *history[MAX_HISTORY];
int history_count = 0;  // Current count of history entries

// Arguments used by shell commands
char *args[MAX_LINE / 2 + 1];
char cwd[MAX_LINE];  // Current working directory
char line[MAX_LINE];  // Line entered by the user

/**
 * @brief Starts the shell loop, continuously reading user input.
 */
void shell() {
    // Set up a signal handler for Ctrl+C (SIGINT)
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    signal(SIGTERM, handle_sigterm);

    while (1) {
        // Print the current directory as a prompt
        printf("%s> ", get_path());

        // Read the user input
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            break;  // End of input or error
        } 
        else {
            add_to_history(line);  // Add the command to history
            execute_command(line);  // Execute the command entered
            printf("\n");
        }
    }    
}

/**
 * @brief Handles the Ctrl+C (SIGINT) signal to prevent shell termination.
 */
void handle_sigint(int sig) {
    printf("\n");  // Print a newline for neatness
    fflush(stdout);

    // Print the prompt again after interrupting with Ctrl+C
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s> ", cwd);
        fflush(stdout);
    }
}

/**
 * @brief Handles the Ctrl+Z (SIGTSTP) signal.
 */
void handle_sigtstp(int sig) {
    printf("\n");  // Print a newline for neatness
    fflush(stdout);

    // Print the prompt again after interrupting with Ctrl+Z
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s> ", cwd);
        fflush(stdout);
    }
}

/**
 * @brief Handles the SIGTERM signal to terminate the shell gracefully.
 */
void handle_sigterm(int sig) {
    clear_history();  // Free memory for the command history
    printf("Shell terminated.\n");
    exit(EXIT_SUCCESS);
}

/**
 * @brief Adds a command to the history list.
 * 
 * @param command The command to be added to history.
 */
void add_to_history(char *command) {
    size_t length = strlen(command);
    if (length > 0 && command[length - 1] == '\n') {
        command[length - 1] = '\0';  // Remove trailing newline
    }

    if (history_count < MAX_HISTORY) {
        history[history_count++] = strdup(command);
    } else {
        // Shift history entries to make space for the new command
        free(history[0]);
        for (int i = 1; i < MAX_HISTORY; i++) {
            history[i - 1] = history[i];
        }
        history[MAX_HISTORY - 1] = strdup(command);
    }
}

/**
 * @brief Prints the history of commands entered.
 */
void print_history() {
    printf("Command history:\n");
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

/**
 * @brief Frees all memory allocated to the history list.
 */
void clear_history() {
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }
    history_count = 0;
}

/**
 * @brief Executes a command entered by the user.
 * 
 * @param command The command to be executed.
 */
void execute_command(char *command) {
    char *commands[MAX_LINE];
    char *token;
    char *saveptr;
    int fd;
    int background = 0;
    int num_pipes = 0;
    int i = 0;

    // Split the command into sub-commands using the '&&' delimiter
    token = strtok_r(command, "&&", &saveptr);
    while (token != NULL) {
        commands[num_pipes++] = token;
        token = strtok_r(NULL, "&&", &saveptr);
    }
    commands[num_pipes] = NULL;

    for (i = 0; i < num_pipes; i++) {
        char *current_command = commands[i];

        // Split the current command into sub-commands using the '|' delimiter
        char *pipe_commands[MAX_LINE];
        char *pipe_token;
        char *pipe_saveptr;
        int num_pipe_cmds = 0;

        pipe_token = strtok_r(current_command, "|", &pipe_saveptr);
        while (pipe_token != NULL) {
            pipe_commands[num_pipe_cmds++] = pipe_token;
            pipe_token = strtok_r(NULL, "|", &pipe_saveptr);
        }
        pipe_commands[num_pipe_cmds] = NULL;

        int pipefd[2 * (num_pipe_cmds - 1)];
        for (int j = 0; j < num_pipe_cmds - 1; j++) {
            if (pipe(pipefd + j * 2) < 0) {
                perror("pipe error");
                return;
            }
        }

        int stdout_copy = dup(STDOUT_FILENO);  // Save original stdout
        int stdin_copy = dup(STDIN_FILENO);    // Save original stdin

        int j = 0;
        for (j = 0; j < num_pipe_cmds; j++) {
            // Tokenize each sub-command to get the arguments
            char *sub_args[MAX_LINE / 2 + 1];
            char *sub_token;
            char *sub_saveptr;
            int arg_index = 0;

            sub_token = strtok_r(pipe_commands[j], " \t\n", &sub_saveptr);
            while (sub_token != NULL) {
                // Check if it is an environment variable to evaluate
                if (sub_token[0] == '$') {
                    char *env_value = getenv(sub_token + 1);
                    if (env_value) {
                        sub_args[arg_index++] = env_value;
                    } else {
                        printf("Undefined environment variable: %s\n", sub_token);
                        return;
                    }
                } 
                else if (strcmp(sub_token, "&") == 0) {
                    // Handle background processes
                    background = 1;
                } 
                else if (strcmp(sub_token, ">") == 0) {
                    // Output redirection
                    sub_token = strtok_r(NULL, " \t\n", &sub_saveptr);
                    fd = open(sub_token, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) {
                        perror("open error");
                        return;
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                } 
                else if (strcmp(sub_token, ">>") == 0) {
                    // Output redirection in append mode
                    sub_token = strtok_r(NULL, " \t\n", &sub_saveptr);
                    fd = open(sub_token, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd == -1) {
                        perror("open error");
                        return;
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                } 
                else if (strcmp(sub_token, "<") == 0) {
                    // Input redirection
                    sub_token = strtok_r(NULL, " \t\n", &sub_saveptr);
                    fd = open(sub_token, O_RDONLY);
                    if (fd == -1) {
                        perror("open error");
                        return;
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                } 
                else {
                    sub_args[arg_index++] = sub_token;
                }
                sub_token = strtok_r(NULL, " \t\n", &sub_saveptr);
            }
            sub_args[arg_index] = NULL;

            if (arg_index == 0) return;

            // Handle internal commands (cd, help, history, exit)
            if (strcmp(sub_args[0], "cd") == 0) {
                change_directory(sub_args);
                continue;
            } 
            else if (strcmp(sub_args[0], "help") == 0) {
                print_help();
                continue;
            } 
            else if (strcmp(sub_args[0], "history") == 0) {
                print_history();
                continue;
            } 
            else if (strcmp(sub_args[0], "exit") == 0) {
                exit_shell();
            }
            
            pid_t pid = fork();
            if (pid == 0) {
                // Redirect pipes
                if (j != 0) {
                    dup2(pipefd[(j - 1) * 2], 0);
                }
                if (j != num_pipe_cmds - 1) {
                    dup2(pipefd[j * 2 + 1], 1);
                }

                // Close all unused file descriptors in the child process
                for (int k = 0; k < 2 * (num_pipe_cmds - 1); k++) {
                    close(pipefd[k]);
                }

                execvp(sub_args[0], sub_args);
                perror("execvp error"); // In case of failure
                exit(EXIT_FAILURE);
            } 
            else if (pid < 0) {
                perror("fork error");
                return;
            }
        }

        // Close all file descriptors in the parent process
        for (int k = 0; k < 2 * (num_pipe_cmds - 1); k++) {
            close(pipefd[k]);
        }

        // Wait for all child processes
        for (int k = 0; k < num_pipe_cmds; k++) {
            if (!background) {
                wait(NULL);
            }
        }

        // Restore original stdout and stdin
        dup2(stdout_copy, STDOUT_FILENO);
        dup2(stdin_copy, STDIN_FILENO);
    }
}

/**
 * @brief Prints available shell commands.
 */
void print_help() {
    printf("List of available commands:\n");
    printf("    history : Show command history\n");
    printf("    exit : Exit the shell\n");
    printf("    help : Display this help message\n");
    printf("    help_server : Display help message for the server\n");
    printf("\n");
}

/**
 * @brief Exits the shell gracefully.
 */
void exit_shell() {
    clear_history();
    printf("Shell closed successfully...\n\n");
    exit(EXIT_SUCCESS);
}

/**
 * @brief Changes the current directory.
 * 
 * @param args Array of command arguments. If no argument is provided, changes to HOME directory.
 */
void change_directory(char **args) {
    if (args[1] == NULL) {
        if (chdir(getenv("HOME")) != 0) {
            perror("chdir error");
        }
    } else {
        if (chdir(args[1]) != 0) {
            perror("chdir error");
        }
    }
}

/**
 * @brief Gets the current working directory.
 * 
 * @return A pointer to the current working directory string.
 */
char *get_path() {
    static char cwd[MAX_LINE];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return cwd;
    }  
    else {
        perror("getcwd error");
        return NULL;
    }
}
