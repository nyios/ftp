#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "handlers.h"
#include "server.h"

void handle_user(char *argument, int client_socket) {
    if (strcmp(argument, "julia") == 0) {
        send_response(client_socket, "331 Please specify the password.\r\n");
    }
    else 
        send_response(client_socket, "430 invalid username or password.\r\n");
}

void handle_password(char *argument, int client_socket){
    send_response(client_socket, "230 Login successful.\r\n");
}

void handle_passive(char *argument, int client_socket){
    int ip[4];
    getip(client_socket, ip);
    char response [64];
    if (!state.pasv) {
        state.pasv_socket = create_socket(DATAPORT);
        state.pasv = true;
    }
    snprintf(response, 64, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", ip[0], ip[1], ip[2], ip[3], DATAPORT/256, DATAPORT%256);
    send_response(client_socket, response);

}

void handle_quit(char *argument, int client_socket){
    if (state.pasv)
        close(state.pasv_socket);
    send_response(client_socket, "221 Goodbye.\r\n");
}

void handle_syst(char *argument, int client_socket){
    send_response(client_socket, "UNIX Type: I\r\n");
}

void handle_list(char *argument, int client_socket){
    DIR *d;
    struct dirent *dir;
    char* path = strcmp(argument, "") == 0 ? "." : argument;
    d = opendir(path);
    if (d) {
        send_response(client_socket, "150 Here comes the directory listing.\r\n");
        int connection;
        struct sockaddr_in connection_addr;
        socklen_t connection_addr_len = sizeof(connection_addr);
        if ((connection = accept(state.pasv_socket, (struct sockaddr *)&connection_addr, &connection_addr_len)) < 0) {
            perror("error while accepting data connection");
        }
        while ((dir = readdir(d)) != NULL) {
            send_response(connection, dir->d_name);
            send_response(connection, "\r\n");
        }
        closedir(d);
        send_response(client_socket, "226 Directory send OK.\r\n");
        close(connection);
    } else {
        send_response(client_socket, "550 Failed to open directory.\r\n");
    }

}

void handle_retrieve(char *argument, int client_socket){
    char buffer[BUFFER_SIZE];
    int file_fd = open(argument, O_RDONLY);
    if (file_fd < 0) {
        send_response(client_socket, "550 Failed to open file.\r\n");
    } else {
        send_response(client_socket, "150 Opening data connection.\r\n");
        int bytes;
        int connection;
        struct sockaddr_in connection_addr;
        socklen_t connection_addr_len = sizeof(connection_addr);
        if ((connection = accept(state.pasv_socket, (struct sockaddr *)&connection_addr, &connection_addr_len)) < 0) {
            perror("error while accepting data connection");
        }
        while ((bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
            send(connection, buffer, bytes, 0);
        }
        close(file_fd);
        send_response(client_socket, "226 Transfer complete.\r\n");
        close(connection);
    }

}

void handle_type(char *argument, int client_socket){
    send_response(client_socket, "200 Switching to binary mode.\r\n");
}
