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
#include <regex.h>
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
int resolve_client_list(clientDetails_t* buffer_out, const char* alias_list, int sender_socket);

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
    char shutdown_msg[MAX_MESSAGE_LEN];
    snprintf(shutdown_msg, sizeof(shutdown_msg), "%s", QUIT);
    broadcast_message(shutdown_msg, SERVER_ALIAS);
    printf(YELLOW BOLD "\nInfo: " RESET "Shutting down server...\n");
    close(server_id);
    printf(GREEN BOLD "Success: " RESET "Server socket closed. Exiting.\n");
    exit(0);
}

void* handle_client(void* client_details) {
    clientDetails_t details = *(clientDetails_t*)client_details;
    int sock = details.client_socket;
    int client_id = details.client_id;

    char alias[MAX_USERNAME_LEN];
    bool connected = false;
    send(sock, SERVER_ALIAS ": Please enter your alias? ", BUFFER_SIZE, 0);
    regex_t name_format;
    regcomp(&name_format, "^[A-Za-z_]+$", 0);
    do {
        strcpy(alias, "");
        ssize_t bytes_received = recv(sock, alias, MAX_USERNAME_LEN, 0);
        if (bytes_received <= 0) {
            printf(RED BOLD "\aError: " RESET "Failed to receive alias from client %d.\n", client_id + 1);
            break;
        }

        alias[strcspn(alias, "\n")] = 0;
        
        if (regexec(&name_format, alias, 0, NULL, 0) != 0) {
            printf(YELLOW BOLD "Warning: " RESET "invalid username attempt.");
            char msg[BUFFER_SIZE];
            snprintf(msg, sizeof(msg), SERVER_ALIAS ": " RED BOLD "\a[Error!] " RESET "Invalid alias format ensure (Aa-Zz,_).\n");
            send(sock, msg, strlen(msg), 0);
            continue;
        }

        if (strlen(alias) < MIN_USERNAME_LEN || strlen(alias) >= MAX_USERNAME_LEN) {
            char msg[BUFFER_SIZE];
            snprintf(msg, sizeof(msg), SERVER_ALIAS ": " RED BOLD "\a[Error!] " RESET "Invalid alias. Ensure your character is count (%d-%d).\n", MIN_USERNAME_LEN, MAX_USERNAME_LEN - 1);
            send(sock, msg, strlen(msg), 0);
        } else {
            for (int i = 0; i < client_count; i++) {
                if (i != client_id && strcmp(CLIENTS[i].client_alias, alias) == 0) {
                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg), SERVER_ALIAS ": " RED BOLD "\a[Error!] " RESET "Alias '%s' is already taken.\n", alias);
                    send(sock, msg, strlen(msg), 0);
                    alias[0] = '\0'; // Invalidate alias
                    break;
                }
            }
            if (alias[0] != '\0')
                connected = true;
        }
    } while (!connected);
    regfree(&name_format);
    strncpy(CLIENTS[client_id].client_alias, alias, MAX_USERNAME_LEN);
    printf(GREEN BOLD "Success: " RESET "Client %d set alias to " YELLOW "'%s'.\n" RESET, client_id + 1, alias);
    char join_msg[MAX_MESSAGE_LEN] = {0};
    snprintf(join_msg, sizeof(join_msg), "%s has joined the chat!", alias);
    broadcast_message(join_msg, SERVER_ALIAS);

    while (connected) {
        char buffer[BUFFER_SIZE] = {0};
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
    char leave_msg[MAX_MESSAGE_LEN] = {0};
    snprintf(leave_msg, sizeof(leave_msg), "%s has left the chat.", alias);
    broadcast_message(leave_msg, SERVER_ALIAS);
    printf(YELLOW BOLD "Info: " RESET "Connection with client " YELLOW "'%s'" RESET " closed.\n", alias);
    return NULL;
}

void broadcast_message(const char* message, const char* sender_alias) {
    /*
    *message format:
    *   <INCLUDE|EXCLUDE> alias1,alias2,... message_content
    */

    int sender_socket = 0;
    if (!strcmp(sender_alias, SERVER_ALIAS) == 0) {
        for (int i = 0; i < client_count; i++) {
            if (strcmp(CLIENTS[i].client_alias, sender_alias) == 0) {
                sender_socket = CLIENTS[i].client_socket;
                break;
            }
        }
    }
    int white_list_count = MAX_CLIENTS;
    clientDetails_t WHITE_LIST[MAX_CLIENTS] = {0};

    char message_copy[BUFFER_SIZE] = {0};
    char filtered_message[MAX_MESSAGE_LEN] = {0};
    strncpy(message_copy, message, BUFFER_SIZE);
    char* control_sequence = strtok(message_copy, " ");

    if (strncmp(control_sequence, INCLUDE, strlen(INCLUDE)) == 0) {
        printf(YELLOW BOLD "Info: " RESET "Inclusion broadcast invoked.\n");
        char* include_list = strtok(message_copy + (strlen(INCLUDE) + 1), " ");
        snprintf(filtered_message, sizeof(filtered_message), BLUE BOLD "[@]" RESET "%s", message + strlen(control_sequence) + strlen(include_list) + 2);
        if (resolve_client_list(WHITE_LIST, include_list, sender_socket) != 2) {
            for (int i = 0; i < white_list_count; i++) {
                if (strcmp(WHITE_LIST[i].client_alias, "") == 0) {
                    white_list_count = i;
                    break;
                }
                printf(YELLOW BOLD "Info: " RESET "verified alias '%s'\n" ,WHITE_LIST[i].client_alias); 
            }
        } else {
            return;
        }
    } else if (strncmp(control_sequence, EXCLUDE, strlen(EXCLUDE)) == 0) {
        printf(YELLOW BOLD "Info: " RESET "Exclusion broadcast invoked.\n");
        char* exclude_list = strtok(message_copy + (strlen(EXCLUDE) + 1), " ");
        snprintf(filtered_message, sizeof(filtered_message), BLUE BOLD "[@]" RESET "%s", message + strlen(control_sequence) + strlen(exclude_list) + 2);
        clientDetails_t BLACK_LIST[MAX_CLIENTS] = {0};
        int r = resolve_client_list(BLACK_LIST, exclude_list, sender_socket);
        if (r == 0) {
            white_list_count = 0;
            for (int i = 0; i < client_count; i++) {
                printf("aliases: %s, %s\n", CLIENTS[i].client_alias, BLACK_LIST[i].client_alias);
                char* alias = CLIENTS[i].client_alias;
                bool exclude = false;
                for (int j=0; j<client_count; j++) {
                    if (strcmp(alias, BLACK_LIST[j].client_alias) == 0) {
                        printf(YELLOW BOLD "Info: " RESET "excluded alias '%s'\n" ,BLACK_LIST[i].client_alias);
                        exclude = true;
                        break;
                    } else if (strcmp(BLACK_LIST[j].client_alias, "") == 0) {
                        break;
                    }
                }
                if (!exclude) {
                    WHITE_LIST[white_list_count++] = CLIENTS[i];
                }
            }
        } else if (r == 1) {
            snprintf(filtered_message, sizeof(filtered_message), RED BOLD "[Error!]" RESET " text not delivered");
            char formatted_message[BUFFER_SIZE] = {0};
            snprintf(formatted_message, sizeof(formatted_message), "%s: %s", SERVER_ALIAS, filtered_message);
            send(sender_socket, formatted_message, sizeof(formatted_message), 0);
            printf(YELLOW BOLD "Info: " RESET "skipping exclude broadcast from '%s'\n", sender_alias);
            return;
        } else {
            return;
        }
    } else {
        memcpy(WHITE_LIST, CLIENTS, sizeof(CLIENTS));
        strncpy(filtered_message, message, MAX_MESSAGE_LEN);
    }

    const char* sender = strcmp(sender_alias, SERVER_ALIAS) == 0 ? "(server)" : sender_alias;
    printf(YELLOW BOLD "Info: " RESET "Broadcasting message %s from '%s'\n", filtered_message, sender);
    char formatted_message[BUFFER_SIZE] = {0};
    snprintf(formatted_message, sizeof(formatted_message), "%s: %s", sender_alias, filtered_message);
    for (int i = 0; i < white_list_count; i++) {
        send(WHITE_LIST[i].client_socket, formatted_message, strlen(formatted_message), 0);
    }
}

int resolve_client_list(clientDetails_t* buffer_out, const char* alias_list, int sender_socket) {
    /*
    * alias_list format:
    *   alias1,alias2,alias3,...
    */

    int warning = 0;
    char list[strlen(alias_list)];
    strncpy(list, alias_list, strlen(alias_list));
    int count = 0;
    int list_len = 0;
    char invalid_aliases[client_count*(MAX_USERNAME_LEN+1)];
    strcpy(invalid_aliases, "");
    
    if (strcmp(&alias_list[strlen(alias_list) - 1], ",") == 0) {
        printf(RED BOLD "Error: " RESET "invalid syntax\n");
        strcat(invalid_aliases, "invalid syntax");
        warning = 2;
    } else if (!strstr(alias_list, ",")) {
        const char* alias = alias_list;
        bool client_found = false;
        for (int i=0;i<client_count;i++) {
            if (strcmp(CLIENTS[i].client_alias, alias) == 0) {
                buffer_out[0] = CLIENTS[i];
                client_found = true;
                break;
            }
        }
        if (!client_found) {
            printf(YELLOW BOLD "Warning: " RESET "Alias '%s' not found in client list. Skipping.\n", alias);
            char user_name[MAX_USERNAME_LEN] = {0};
            strcat(user_name, alias);
            strcat(invalid_aliases, user_name);
            warning = 1;
        }
    } else {
        while (list_len <= (int) strlen(alias_list)) {
            char new_list[strlen(alias_list)];
            snprintf(new_list, sizeof(new_list), "%s", list + list_len);
            char* alias = strtok(new_list, ",");
            if (list_len + strlen(alias) > strlen(alias_list)) {
                snprintf(new_list, sizeof(new_list), "%s", alias_list + list_len);
                alias = strtok(new_list, "");
            }
            list_len += strlen(alias) + 1;
            
            bool client_found = false;
            for (int i = 0; i < client_count; i++) {
                if (strcmp(CLIENTS[i].client_alias, alias) == 0) {
                    buffer_out[count++] = CLIENTS[i];
                    client_found = true;
                    break;
                }
            }
            if (!client_found) {
                printf(YELLOW BOLD "Warning: " RESET "Alias '%s' not found in client list. Skipping.\n", alias);
                char user_name[MAX_USERNAME_LEN] = {0};
                strcat(user_name, alias);
                strcat(user_name, ", ");
                strcat(invalid_aliases, user_name);
                warning = 1;
            }
        }
    }

    char return_message[BUFFER_SIZE];
    if (warning == 0) {
        char message[MAX_MESSAGE_LEN];
        snprintf(message, sizeof(message), GREEN BOLD "[success!] " RESET "aliases verified.\n");
        snprintf(return_message, sizeof(return_message), "%s: %s", SERVER_ALIAS, message);
    } else if (warning == 1) {
        char message[MAX_MESSAGE_LEN];
        snprintf(message, sizeof(message), YELLOW BOLD "[Warning!] " RESET "aliases (%s) not found\n" , invalid_aliases);
        snprintf(return_message, sizeof(return_message), "%s: %s", SERVER_ALIAS, message);
    } else {
        char message[MAX_MESSAGE_LEN];
        snprintf(message, sizeof(message), RED BOLD "[Error!] " RESET "%s\n" , invalid_aliases);
        snprintf(return_message, sizeof(return_message), "%s: %s", SERVER_ALIAS, message);
    }
    
    send(sender_socket, return_message, strlen(return_message), 0);
    return warning;
}

