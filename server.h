#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_LINE 1024
#define PORT 2580
#define MAX_CLIENTS 10

void server(int port);
void multi_server(int port);
void exit_server();
void print_server_help();

#endif