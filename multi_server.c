#include "shell.h"
#include "server.h"

typedef struct {
    int socket_fd;
    struct sockaddr_in address;
} ClientInfo;

/**
 * @brief Starts the multi-client server on the specified port.
 * 
 * The server listens for incoming client connections and handles
 * multiple clients concurrently. Commands can be executed locally
 * or sent to specific/all clients.
 *
 * @param port The port number on which the server will listen.
 */
void multi_server(int port) {
    char buffer[MAX_LINE] = {0};
    int fd_server, new_socket;
    int opt = 1;
    ClientInfo client_sockets[MAX_CLIENTS] = {0};  // Array to store client information
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Handle signals
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    signal(SIGTERM, handle_sigterm);

    // Create server socket
    if ((fd_server = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow reuse of address
    if (setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt error");
        close(fd_server);
        exit(EXIT_FAILURE);
    }

    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind socket to address
    if (bind(fd_server, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind error");
        close(fd_server);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(fd_server, MAX_CLIENTS) < 0) {
        perror("listen error");
        close(fd_server);
        exit(EXIT_FAILURE);
    }

    fd_set readfds;
    int max_fd = fd_server;

    while (1) {
        printf("\nMulti-client server waiting for connections (PORT: %d)...\n", port);
        printf("\n%s> ", get_path());
        fflush(stdout);

        FD_ZERO(&readfds);
        FD_SET(fd_server, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        // Add existing client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int socket = client_sockets[i].socket_fd;
            if (socket > 0) {
                FD_SET(socket, &readfds);
            }
            if (socket > max_fd) {
                max_fd = socket;
            }
        }

        // Monitor sockets for activity
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            perror("select error");
            continue;
        }

        // Check for new client connections
        if (FD_ISSET(fd_server, &readfds)) {
            if ((new_socket = accept(fd_server, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("accept error");
                continue;
            }
            printf("\nNew connection, socket fd: %d, IP: %s, PORT: %d\n",
                   new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // Add the new socket to the list of clients
            int added = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i].socket_fd == 0) {
                    client_sockets[i].socket_fd = new_socket;
                    client_sockets[i].address = address;
                    added = 1;
                    break;
                }
            }

            if (!added) {
                printf("Maximum number of clients reached. Closing connection: %d\n", new_socket);
                close(new_socket);
            }
        }

        // Check if a command was entered via the server console
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, MAX_LINE + 1);
            if (fgets(buffer, MAX_LINE, stdin) != NULL) {
                buffer[strcspn(buffer, "\n")] = 0;  // Remove newline character

                if (strlen(buffer) > 0) {
                    if (strcmp(buffer, "help_server") == 0) {
                        // Display server help commands
                        printf("\n");
                        print_server_help();
                        printf("\n");
                    } 
                    else if (strcmp(buffer, "exit_server") == 0) {
                        // Notify clients about server shutdown and exit
                        exit_server();
                    } 
                    else if (strcmp(buffer, "list_clients") == 0) {
                        // List all connected clients
                        printf("\nList of connected clients:\n");
                        for (int i = 0; i < MAX_CLIENTS; i++) {
                            if (client_sockets[i].socket_fd > 0) {
                                printf("Client socket fd: %d, IP: %s, PORT: %d\n",
                                       client_sockets[i].socket_fd,
                                       inet_ntoa(client_sockets[i].address.sin_addr),
                                       ntohs(client_sockets[i].address.sin_port));
                            }
                        }
                        printf("\n");
                    } else {
                        int send_to_all = 0;
                        int send_to_specific = 0;
                        char *all_flag = strstr(buffer, "-all");
                        char *id_flag = strstr(buffer, "-id");

                        // Determine if the command should be sent to all or specific clients
                        if (all_flag != NULL) {
                            send_to_all = 1;
                            *all_flag = '\0';
                            if (all_flag > buffer && *(all_flag - 1) == ' ') {
                                *(all_flag - 1) = '\0';
                            }
                        } 
                        else if (id_flag != NULL) {
                            send_to_specific = 1;
                        }

                        if (send_to_all) {
                            // Send the command to all connected clients
                            for (int i = 0; i < MAX_CLIENTS; i++) {
                                int sock = client_sockets[i].socket_fd;
                                if (sock > 0) {
                                    if (send(sock, buffer, strlen(buffer) + 1, 0) == -1) {
                                        perror("send error");
                                        close(sock);
                                        client_sockets[i].socket_fd = 0;
                                    } 
                                    else {
                                        printf("Command sent to client socket %d: %s\n", sock, buffer);
                                    }
                                }
                            }
                        } 
                        else if (send_to_specific) {
                            // Send the command to specific clients by socket ID
                            char *token = strtok(buffer, " ");
                            while (token != NULL) {
                                if (strcmp(token, "-id") == 0) {
                                    token = strtok(NULL, " ");
                                    if (token != NULL) {
                                        int target_fd = atoi(token);
                                        for (int i = 0; i < MAX_CLIENTS; i++) {
                                            if (client_sockets[i].socket_fd == target_fd) {
                                                if (send(target_fd, buffer, strlen(buffer) + 1, 0) == -1) {
                                                    perror("send error");
                                                    close(target_fd);
                                                    client_sockets[i].socket_fd = 0;
                                                } else {
                                                    printf("Command sent to client socket %d: %s\n", target_fd, buffer);
                                                }
                                            }
                                        }
                                    }
                                }
                                token = strtok(NULL, " ");
                            }
                        } 
                        else {
                            // Execute the command locally by default
                            execute_command(buffer);
                        }
                    }
                    add_to_history(buffer);
                }
            }
        }

        // Handle responses from clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sock = client_sockets[i].socket_fd;
            if (sock > 0 && FD_ISSET(sock, &readfds)) {
                memset(buffer, 0, MAX_LINE);
                int valread = read(sock, buffer, MAX_LINE);
                if (valread > 0) {
                    // Print response from the client
                    printf("\nResponse from client socket %d: %s\n", sock, buffer);
                } 
                else if (valread == 0) {
                    // Handle client disconnection
                    printf("\nClient socket %d disconnected\n", sock);
                    close(sock);
                    client_sockets[i].socket_fd = 0;
                } 
                else {
                    perror("read error");
                    close(sock);
                    client_sockets[i].socket_fd = 0;
                }
            }
        }
    }

    // Close all client sockets before closing the server socket
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i].socket_fd > 0) {
            close(client_sockets[i].socket_fd);
        }
    }
    printf("Closing server socket...\n");
    close(fd_server);
}
