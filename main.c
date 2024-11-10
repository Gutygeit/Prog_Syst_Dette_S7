#include "shell.h"
#include "server.h"
#include "client.h"

/**
 * @brief Entry point for the application.
 * 
 * This function handles different modes of the application based on the arguments provided.
 * It can start a shell, a server, a client, or a multi-client server.
 *
 * @param argc The number of arguments passed to the program.
 * @param argv The array of arguments passed to the program.
 * @return int Status code indicating success or failure.
 */
int main(int argc, char *argv[]) {
    // Check if the required argument is provided
    if (argc < 2) {
        printf("Missing argument... %s\n", argv[0]);
        printf("Available arguments: <shell>, <server>, <client>, <multi_server>\n");
        return 1;
    }

    int port = 0;

    // Check if a port is provided
    if (argc == 3) {
        port = atoi(argv[2]);  // Convert the given port argument
    }

    // Handle different arguments
    if (strcmp(argv[1], "shell") == 0) {
        // Start the shell mode
        shell();
    }
    else if (strcmp(argv[1], "server") == 0) {
        // Start the server mode
        if (port == 0) {
            printf("Please specify a port for the server (>1234).\n");
            return 1;
        }
        server(port);
    }
    else if (strcmp(argv[1], "client") == 0) {
        // Start the client mode
        if (port == 0) {
            printf("Please specify a port for the client (>1234).\n");
            return 1;
        }
        client(port);
    }
    else if (strcmp(argv[1], "multi_server") == 0) {
        // Start the multi-client server mode
        if (port == 0) {
            printf("Please specify a port for the multi_server (>1234).\n");
            return 1;
        }
        multi_server(port);
    }
    else {
        // If an unknown argument is provided
        printf("Unknown argument... %s\n", argv[1]);
        printf("Available arguments: <shell>, <server>, <client>, <multi_server>\n");
    }

    return 0;
}
