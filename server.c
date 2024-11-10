#include "shell.h"
#include "server.h"

/**
 * @brief Prints the available server commands and their usage.
 */
void print_server_help() {
    printf("Available commands:\n");
    printf("  exit : Close the server (local mode) or disconnect a client (connected mode)\n");
    printf("  exit_server: Shut down the server (connected mode)\n");
    printf("  exit_client: Disconnect a client from the server (connected mode)\n");
    printf("  list_clients: List all currently connected clients (multi_server mode only)\n");
    printf("  help_server: Display this help message\n");
    printf("\nFor multi_server mode:\n");
    printf("  Commands execute locally by default.\n");
    printf("  Use '-all' to send a command to all clients, or '-id <x>' to target specific client(s).\n");
}

/**
 * @brief Shuts down the server gracefully.
 */
void exit_server() {
    printf("\nShutting down the server...\n\n");
    exit(EXIT_SUCCESS);
}

/**
 * @brief Starts the server on the specified port, accepting connections and executing commands.
 * 
 * @param port The port number on which the server will listen for connections.
 */
void server(int port) {
    char buffer[MAX_LINE] = {0};
    int fd_server, new_socket;
    int opt = 1;
    struct sockaddr_in adresse;
    int addrlen = sizeof(adresse);

    // Handle signals
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    signal(SIGTERM, handle_sigterm);

    // Create server socket
    if ((fd_server = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address
    if (setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    adresse.sin_family = AF_INET;
    adresse.sin_addr.s_addr = INADDR_ANY;
    adresse.sin_port = htons(port);

    // Bind socket to the address
    if (bind(fd_server, (struct sockaddr *)&adresse, sizeof(adresse)) < 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(fd_server, 3) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    fd_set readfds;
    int max_fd = fd_server;

    while (1) {
        printf("\nServer waiting for connection (PORT: %d)...\n", port);
        printf("\n%s> ", get_path());
        fflush(stdout);

        // Reset the file descriptor set
        FD_ZERO(&readfds);
        FD_SET(fd_server, &readfds);  // Add server socket to set
        FD_SET(STDIN_FILENO, &readfds);  // Add stdin for local commands

        // Use select to monitor connections and local inputs
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select error");
            continue;
        }
        
        // Check if a local command was entered
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, MAX_LINE + 1);
            if (fgets(buffer, MAX_LINE, stdin) != NULL) {
                buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
                if (strcmp(buffer, "help_server") == 0) {
                    printf("\n");
                    print_server_help();  // Display help for server commands
                    printf("\n");
                } else {
                    execute_command(buffer);  // Execute the local command
                }
                add_to_history(buffer);
            }
        }

        // Check if a client is trying to connect
        if (FD_ISSET(fd_server, &readfds)) {
            if ((new_socket = accept(fd_server, (struct sockaddr *)&adresse, (socklen_t *)&addrlen)) < 0) {
                perror("accept error");
                continue;
            }

            printf("\nConnection established with client\n");

            // Communication loop with the client
            while (1) {
                printf("\nEnter command to send to the client (type 'exit_client' to disconnect):\n");
                printf("Server> ");
                memset(buffer, 0, MAX_LINE + 1);
                if (fgets(buffer, MAX_LINE, stdin) == NULL) {
                    break;  // Exit if input fails
                }

                buffer[strcspn(buffer, "\n")] = 0;  // Remove newline character

                if (strcmp(buffer, "exit_client") == 0) {
                    printf("\nDisconnecting from client...\n");
                    break;  // Exit communication loop
                }

                if (strcmp(buffer, "exit_server") == 0) {
                    printf("\nShutting down the server...\n");
                    close(fd_server);
                    exit(EXIT_SUCCESS);
                }

                // Check if the command contains '-local' for local execution
                char *local_suffix = strstr(buffer, "-local");
                if (local_suffix != NULL) {
                    *local_suffix = '\0';  // Remove '-local' from the command
                    while (buffer[strlen(buffer) - 1] == ' ') {
                        buffer[strlen(buffer) - 1] = '\0';  // Remove trailing spaces
                    }
                    execute_command(buffer);  // Execute locally
                    add_to_history(buffer);
                } 
                else {
                    // Send the command to the client
                    if (send(new_socket, buffer, strlen(buffer), 0) == -1) {
                        perror("send error");
                        break;
                    }

                    // Read the client's response
                    memset(buffer, 0, MAX_LINE + 1);
                    int valread = read(new_socket, buffer, MAX_LINE);
                    if (valread > 0) {
                        printf("\nClient response: %s\n", buffer);
                    } 
                    else if (valread == 0) {
                        printf("\nClient has closed the connection.\n");
                        break;  // Exit loop if client disconnects
                    } 
                    else {
                        perror("read error");
                        break;  // Exit loop on read error
                    }
                }
            }

            // Close the client socket after disconnection
            close(new_socket);
            printf("\nReturned to local mode, awaiting new connections or local commands.\n");
        }
    }

    close(fd_server);
}
