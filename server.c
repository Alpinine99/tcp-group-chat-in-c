#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "commons.h"


typedef struct {
    int client_socket;
    int client_id;
} clientDetails_t;

void close_server();
void* handle_client(void* client_details);

int server_id;
int client_count = 0;
int client_sockets[MAX_CLIENTS];
pthread_t clients_threads[MAX_CLIENTS];

int main(int argc, char* argv[]) {

    unsigned int port;
    if (argc < 2) {
        printf(RED BOLD "\aError: " RESET "No port number provided.\n");
        return 1;
    } else {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            printf(RED BOLD "\aError: " RESET "Invalid port number '%s'. Please provide a number between 1 and 65535.\n", argv[1]);
            return 1;
        }
    }
    printf(GREEN BOLD "Success: " RESET "Starting server on port %d...\n", port);

    if ((server_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf(RED BOLD "\aError: " RESET "Failed to create socket.\n");
        return 1;
    } else {
        printf(GREEN BOLD "Success: " RESET "Socket created with ID %d.\n", server_id);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_id, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf(RED BOLD "\aError: " RESET "Failed to bind socket to port %d.\n", port);
        return 1;
    } else {
        printf(GREEN BOLD "Success: " RESET "Socket bound to port %d.\n", port);
    }

    if (listen(server_id, MAX_CLIENTS) < 0) {
        printf(RED BOLD "\aError: " RESET "Failed to listen on socket.\n");
        return 1;
    } else {
        printf(GREEN BOLD "Success: " RESET "Server is listening on port %d.\n", port);
    }

    signal(SIGINT, close_server);
    for (; client_count < MAX_CLIENTS; ) {
        printf(YELLOW BOLD "Info: " RESET "Waiting for client %d to connect...\n", client_count + 1);
        // Accept client connections here
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket;
        if ((client_socket = accept(server_id, (struct sockaddr*)&client_addr, &client_len)) < 0) {
            printf(RED BOLD "\aError: " RESET "Failed to accept client %d connection.\n", client_count + 1);
        } else {
            printf(GREEN BOLD "Success: " RESET "Client %d connected.\n", client_count + 1);
            // Handle client communication here
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            printf(CYAN BOLD "Info: " RESET "Client %d IP address: %s Port: %d\n", client_count + 1, client_ip, ntohs(client_addr.sin_port));

            // Create a thread to handle the client
            client_sockets[client_count] = client_socket;
            clientDetails_t client_details = {.client_id=client_count, .client_socket=client_sockets[client_count]};
            if (pthread_create(&clients_threads[client_count], NULL, handle_client, (void *)&client_details) != 0) {
                printf(RED BOLD "\aError: " RESET "Failed to create thread for client %d.\n", client_count + 1);
            } else {
                printf(GREEN BOLD "Success: " RESET "Thread created for client %d.\n", client_count + 1);
                client_count++;
            }
        }
    }

    // Join client threads before exiting
    for (int i = 0; i < client_count; i++) {
        pthread_join(clients_threads[i], NULL);
    }
    close_server();
    return 0;
}

void close_server() {
    printf(YELLOW BOLD "\nInfo: " RESET "Shutting down server...\n");
    close(server_id);
    printf(GREEN BOLD "Success: " RESET "Server socket closed. Exiting.\n");
    exit(0);
}

void* handle_client(void* client_details) {
    clientDetails_t details = *(clientDetails_t*)client_details;
    int sock = details.client_socket;
    char buffer[BUFFER_SIZE];
    int client_id = details.client_id;

    while (1) {
        ssize_t bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf(RED BOLD "\aError: " RESET "Failed to receive data from client %d.\n", client_id + 1);
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
        printf("Received message: %s ,From client %d\n", buffer, client_id + 1);

        if (strncmp(buffer, "quit", 4) == 0) {
            printf(YELLOW BOLD "Info: " RESET "Client %d requested to close the connection.\n", client_id + 1);
            break;
        }

        const char* response = "Message received";
        send(sock, response, strlen(response), 0);
    }

    client_count--;
    close(sock);
    printf(YELLOW BOLD "Info: " RESET "Connection with client %d closed.\n", client_id + 1);
    return NULL;
}

