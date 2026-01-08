#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "commons.h"
#include "client_consts.h"


void* handle_sends(void* arg);

int main(int argc, char* argv[]) {
    unsigned int server_port;
    char* server_ip;
    if (argc < 3) {
        printf(RED BOLD"Error: " RESET "Not enough arguments provided.\nUsage: %s <server_ip> <server_port>\n", argv[0]);
        return 1;
    } else {
        server_ip = argv[1];
        server_port = atoi(argv[2]);
        if (server_port <= 0 || server_port > 65535) {
            printf(RED BOLD "Error: " RESET "Invalid port number '%s'. Please provide a number between 1 and 65535.\n", argv[2]);
            return 1;
        }
    }
    printf(YELLOW BOLD "Connecting to server at %s on port %d...\n" RESET, server_ip, server_port);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf(RED BOLD "Error: " RESET "Invalid server IP address '%s'.\n", server_ip);
        return 1;
    }

    int client_id;
    if ((client_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf(RED BOLD "\aError: " RESET "Failed to create socket.\n");
        return 1;
    } else {
        printf(GREEN BOLD "Success: " RESET "Socket created with ID %d.\n", client_id);
    }

    if (connect(client_id, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf(RED BOLD "\aError: " RESET "Failed to connect to server at %s:%d.\n", server_ip, server_port);
        return 1;
    }
    printf(GREEN BOLD "Success: " RESET "Connected to server at %s:%d.\n", server_ip, server_port);

    pthread_t send_thread;
    if (pthread_create(&send_thread, NULL, handle_sends, (void *)&client_id) != 0) {
        printf(RED BOLD "\aError: " RESET "Failed to create send thread.\n");
        return 1;
    }

    while (1) {
        char buffer[BUFFER_SIZE] = {0};
        ssize_t bytes_received = recv(client_id, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf(RED BOLD "\aError: " RESET "Failed to receive data from server.\n");
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
        char* alias = strtok(buffer, ":");
        char* message = strtok(NULL, "");
        if (message != NULL) {
            if (strcmp(alias, SERVER_ALIAS) == 0) {
                printf(RED BOLD "(Server)" RESET "%s\n", message);
            } else {
                printf(CYAN BOLD "%s:" RESET "%s\n", alias, message);
            }
        }
        if (strncmp(buffer, QUIT, strlen(QUIT)) == 0) {
            printf(YELLOW BOLD "Info: " RESET "Server requested to close the connection.\n");
            break;
        }
    }

    pthread_join(send_thread, NULL);

    close(client_id);
    printf(YELLOW BOLD "Info: " RESET "Connection closed.\n");
    return 0;
}

void* handle_sends(void* arg) {
    int sock = *((int*)arg);
    while (1) {
        char buffer[BUFFER_SIZE] = {0};
        printf(GREEN BOLD "You: " RESET);
        fgets(buffer, BUFFER_SIZE, stdin);
        
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
        if (strncmp(buffer, LEAVE_CMD, strlen(LEAVE_CMD)) == 0) {
            printf(YELLOW BOLD "Info: " RESET "You requested to close the connection.\n");
            send(sock, QUIT, strlen(QUIT), 0);
            break;
        }

        char buffer_copy[BUFFER_SIZE] = {0};
        char modified_buffer[BUFFER_SIZE] = {0};
        strcpy(buffer_copy, buffer);
        char* control_sequence = strtok(buffer_copy, " ");
        if (strstr(control_sequence, PRIVATE_MSG_EXCLUDE_CMD)) {
            snprintf(modified_buffer, sizeof(modified_buffer), "%s %s", EXCLUDE, buffer + strlen(PRIVATE_MSG_EXCLUDE_CMD));
        } else if (strstr(control_sequence, PRIVATE_MSG_INCLUDE_CMD)) {
            snprintf(modified_buffer, sizeof(modified_buffer), "%s %s", INCLUDE, buffer + strlen(PRIVATE_MSG_INCLUDE_CMD));
        } else {
            strcpy(modified_buffer, buffer);
        }
        send(sock, modified_buffer, strlen(modified_buffer), 0);
    }
    return NULL;
}

