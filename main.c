#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>

#include "server.h"
#include "handlers.h"

void signal_handler();
void handle_client(int client_socket);

int server_socket;
Server_State state = {false, 0};

void(*func_ptrs [NUM_COMMANDS])(char*, int) = {handle_list, handle_passive, handle_password, 
    handle_retrieve, handle_syst, handle_type, handle_user, handle_quit};
char* commands [NUM_COMMANDS] = {"LIST", "PASV", "PASS", "RETR", "SYST", "TYPE", "USER", "QUIT"};

static int get_index(char* string) {
    for (int i = 0; i < sizeof(commands) / sizeof(char*); ++i) {
        if (strcmp(commands[i], string) == 0) {
            return i;
        }
    }
    return -1;
}

//thread stuff
#define MAX_THREADS 8
typedef struct {
    int client_socket;
    int index;
} thread_data_t;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

thread_data_t thread_data[MAX_THREADS];
pthread_t threads[MAX_THREADS];
int available_threads[MAX_THREADS];
int next_thread = 0;

void* thread_function(void *arg);

int main() {
    int client_socket;
    server_socket = create_socket(PORT);
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    printf("FTP server listening on port %d\r\n", PORT);

    // Initialize the available threads array
    for (int i = 0; i < MAX_THREADS; i++) {
        available_threads[i] = 1;  // All threads are initially available
    }

    // Create worker threads
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_function, (void *)&thread_data[i]);
    }

    while (1) {
        // Wait for a connection
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            perror("error while accepting client connection");
            continue;
        }
        // Find an available thread
        while (!available_threads[next_thread]) {
            next_thread = (next_thread + 1) % MAX_THREADS;
        }
        available_threads[next_thread] = 0;  // Mark the thread as unavailable
        thread_data[next_thread] = (thread_data_t){client_socket, next_thread};
        pthread_cond_signal(&cond);  // Signal the thread
        next_thread = (next_thread + 1) % MAX_THREADS;
    }
    close(server_socket);
    return 0;
}

void* thread_function(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    while (1) {
        pthread_mutex_lock(&lock);
        while(data->client_socket == 0) // Makes sure only the thread chosen by the main thread gets woken
            pthread_cond_wait(&cond, &lock); // Wait on condition variable
        int client_socket = data->client_socket;
        data->client_socket = 0;  // Reset for the next connection
        pthread_mutex_unlock(&lock);

        handle_client(client_socket);

        available_threads[data->index] = 1;  // Mark the thread as available 
    }
    return NULL;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE], argument[BUFFER_SIZE];

    send_response(client_socket, "220 Welcome to the simple FTP server\r\n");

    while (1) {
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (bytes_read < 0) {
            perror("read");
            break;
        }

        buffer[bytes_read] = '\0';
        int ret = sscanf(buffer, "%s %s", command, argument);
        // if only one string was matched set argument to empty string
        if (ret == 1)
            argument[0] = '\0';

        int i = get_index(command);
        if (i < 0) {
            fprintf(stderr, "unknown command %s with argument %s\n", command, argument);
            if (send_response(client_socket, "502 Command not implemented.\r\n") < 0)
                break;
        } else if (strcmp(command, "QUIT") == 0){
            if (state.pasv)
                close(state.pasv_socket);
            send_response(client_socket, "221 Goodbye.\r\n");
            break;
        } else {
            func_ptrs[i](argument, client_socket);
        }
    }
}

