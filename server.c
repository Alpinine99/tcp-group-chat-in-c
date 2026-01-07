#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include "commons.h"


typedef struct {
    int client_socket;
    int client_id;
    pthread_t client_thead;
    char client_alias[MAX_USERNAME_LEN];
} clientDetails_t;

void close_server();
void* handle_client(void* client_details);
void broadcast_message(const char* message, const char* sender_alias);

int server_id;
int client_count = 0;
clientDetails_t CLIENTS[MAX_CLIENTS];

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
        pthread_t client_thread;
        if ((client_socket = accept(server_id, (struct sockaddr*)&client_addr, &client_len)) < 0) {
            printf(RED BOLD "\aError: " RESET "Failed to accept client %d connection.\n", client_count + 1);
        } else {
            printf(GREEN BOLD "Success: " RESET "Client %d connected.\n", client_count + 1);
            // Handle client communication here
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            printf(CYAN BOLD "Info: " RESET "Client %d IP address: %s Port: %d\n", client_count + 1, client_ip, ntohs(client_addr.sin_port));

            // Create a thread to handle the client
            clientDetails_t client_details = {.client_id=client_count, .client_socket=client_socket, .client_thead=client_thread};
            if (pthread_create(&client_thread, NULL, handle_client, (void *)&client_details) != 0) {
                printf(RED BOLD "\aError: " RESET "Failed to create thread for client %d.\n", client_count + 1);
            } else {
                printf(GREEN BOLD "Success: " RESET "Thread created for client %d.\n", client_count + 1);
                CLIENTS[client_count] = client_details;
                client_count++;
            }
        }
    }

    // Join client threads before exiting
    for (int i = 0; i < client_count; i++) {
        pthread_join(CLIENTS[i].client_thead, NULL);
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

    char alias[MAX_USERNAME_LEN];
    bool connected = false;
    send(sock, "Please enter your alias: ", 24, 0);
    do {
        ssize_t bytes_received = recv(sock, alias, MAX_USERNAME_LEN, 0);
        if (bytes_received <= 0) {
            printf(RED BOLD "\aError: " RESET "Failed to receive alias from client %d.\n", client_id + 1);
            break;
        }

        alias[strcspn(alias, "\n")] = 0; // Remove newline character
        if (strlen(alias) < MIN_USERNAME_LEN || strlen(alias) >= MAX_USERNAME_LEN) {
            char msg[BUFFER_SIZE];
            snprintf(msg, sizeof(msg), RED BOLD "\aError: " RESET "Invalid alias. Please try again (character count %d-%d).\n", MIN_USERNAME_LEN, MAX_USERNAME_LEN - 1);
            send(sock, msg, strlen(msg), 0);
        } else {
            for (int i = 0; i < client_count; i++) {
                if (i != client_id && strcmp(CLIENTS[i].client_alias, alias) == 0) {
                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg), RED BOLD "\aError: " RESET "Alias '%s' is already taken. Please choose another one.\n", alias);
                    send(sock, msg, strlen(msg), 0);
                    alias[0] = '\0'; // Invalidate alias
                    break;
                }
            }
            if (alias[0] != '\0')
                connected = true;
        }
    } while (!connected);
    strncpy(CLIENTS[client_id].client_alias, alias, MAX_USERNAME_LEN);
    printf(GREEN BOLD "Success: " RESET "Client %d set alias to " YELLOW "'%s'.\n" RESET, client_id + 1, alias);
    char join_msg[BUFFER_SIZE];
    snprintf(join_msg, sizeof(join_msg), "%s has joined the chat!", alias);
    broadcast_message(join_msg, SERVER_ALIAS);

    while (connected) {
        ssize_t bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf(RED BOLD "\aError: " RESET "Failed to receive data from client" YELLOW " '%s'.\n" RESET, alias);
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0;

        if (strncmp(buffer, QUIT, strlen(QUIT)) == 0) {
            printf(YELLOW BOLD "Info: " RESET "Client " YELLOW "'%s'" RESET " requested to close the connection.\n", alias);
            break;
        } else {
            broadcast_message(buffer, alias);
        }

    }

    // Remove client from CLIENTS array
    for (int i = client_id; i < client_count - 1; i++) {
        CLIENTS[i] = CLIENTS[i + 1];
        CLIENTS[i].client_id = i;
    }
    memset(&CLIENTS[client_count - 1], 0, sizeof(clientDetails_t));

    client_count--;
    close(sock);
    char leave_msg[BUFFER_SIZE];
    snprintf(leave_msg, sizeof(leave_msg), "%s has left the chat.", alias);
    broadcast_message(leave_msg, SERVER_ALIAS);
    printf(YELLOW BOLD "Info: " RESET "Connection with client " YELLOW "'%s'" RESET " closed.\n", alias);
    return NULL;
}

void broadcast_message(const char* message, const char* sender_alias) {
    clientDetails_t WHITE_LIST[MAX_CLIENTS];

    char message_copy[BUFFER_SIZE];
    char filtered_message[MAX_MESSAGE_LEN];
    strncpy(message_copy, message, BUFFER_SIZE);
    char* control_sequence = strtok(message_copy, " ");
    snprintf(filtered_message, sizeof(filtered_message), "%s", message + strlen(control_sequence) + 1);
    if (strncmp(control_sequence, INCLUDE, strlen(INCLUDE)) == 0) {
        printf(YELLOW BOLD "Info: " RESET "Inclusion broadcast invoked.\n");
    } else if (strncmp(control_sequence, EXCLUDE, strlen(EXCLUDE)) == 0) {
        printf(YELLOW BOLD "Info: " RESET "Exclusion broadcast invoked.\n");
    } else {
        memcpy(WHITE_LIST, CLIENTS, sizeof(CLIENTS));
        strncpy(filtered_message, message, MAX_MESSAGE_LEN);
    }

    printf(YELLOW BOLD "Info: " RESET "Broadcasting message %s from '%s'\n", filtered_message, sender_alias);
    for (int i = 0; i < client_count; i++) {
        char formatted_message[BUFFER_SIZE];
        snprintf(formatted_message, sizeof(formatted_message), "%s: %s", sender_alias, filtered_message);
        send(WHITE_LIST[i].client_socket, formatted_message, strlen(formatted_message), 0);
    }
}