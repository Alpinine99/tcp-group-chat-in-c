# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Target executables
SERVER = server
CLIENT = client

# Source files
SERVER_SRC = server.c
CLIENT_SRC = client.c

# Object files
SERVER_OBJ = server.o
CLIENT_OBJ = client.o

# Default target - build both server and client
all: $(SERVER) $(CLIENT)

# Compile server executable
$(SERVER): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_OBJ)

# Compile client executable
$(CLIENT): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_OBJ)

# Compile server source to object file
$(SERVER_OBJ): $(SERVER_SRC)
	$(CC) $(CFLAGS) -c $(SERVER_SRC)

# Compile client source to object file
$(CLIENT_OBJ): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC)

# Clean up generated files
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SERVER) $(CLIENT)

# Phony targets (not actual files)
.PHONY: all clean
