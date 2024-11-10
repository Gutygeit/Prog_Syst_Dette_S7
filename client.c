#include "shell.h"
#include "client.h"

/**
 * @brief Connects the client to the server on a specified port and processes commands.
 * 
 * @param port The port number to connect to the server.
 */
void client(int port) {
    int sockfd = 0;
    char buffer[MAX_LINE] = {0};
    struct sockaddr_in serv_addr;
    
    // Handle signals
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    signal(SIGTERM, handle_sigterm);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet_pton error");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect error");
        exit(EXIT_FAILURE);
    }

    printf("\nConnection established with the server\n");

    // Main loop for interacting with the server or local commands
    while (1) {
        printf("\nWaiting for a command from the server or type your own command...\n\n");
        printf("Client ~ %s> ", get_path());
        fflush(stdout);

        // Use select() to listen for both user input and server commands
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);  // Monitor server socket for incoming data
        FD_SET(STDIN_FILENO, &readfds);  // Monitor standard input for user commands
        int max_fd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select error");
            break;
        }

        // If a command comes from the server
        if (FD_ISSET(sockfd, &readfds)) {
            memset(buffer, 0, MAX_LINE);
            int valread = read(sockfd, buffer, MAX_LINE);
            if (valread > 0) {
                printf("\nCommand received: %s\n", buffer);
                printf("%s> %s\n", get_path(), buffer);
                add_to_history(buffer);
                if (strcmp(buffer, "exit_client") == 0) {
                    exit_client();
                }
                execute_command(buffer);

                // Send response to the server after executing the command
                char *response = "Command executed successfully by the client";
                send(sockfd, response, strlen(response), 0); 
            }
            else if (valread == 0) {
                printf("\nServer has closed the connection.\n");
                break;
            }
            else {
                perror("read error");
                break;
            }
        }

        // If the user enters a command locally
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, MAX_LINE);
            if (fgets(buffer, MAX_LINE, stdin) != NULL) {
                // Remove the newline character
                buffer[strcspn(buffer, "\n")] = 0;

                if (strcmp(buffer, "exit") == 0) {
                    exit_client();  // Gracefully close the client
                }

                add_to_history(buffer);
                execute_command(buffer);  // Execute command locally
            }
        }
    }

    // Close the socket before exiting
    close(sockfd);
}

/**
 * @brief Terminates the client connection gracefully.
 */
void exit_client() {
    clear_history();  // Clear command history before exiting
    printf("Client closed successfully...\n\n");
    exit(EXIT_SUCCESS);
}
