#pragma once
#include <stdbool.h>

#define PORT 2021
#define DATAPORT 2022
#define BUFFER_SIZE 1024

typedef struct Server_State {
    bool pasv;
    int pasv_socket;
} Server_State;

extern Server_State state;

extern int server_socket;

void getip(int sock, int *ip);
int create_socket(int port);
int send_response(int client_socket, const char *response);
