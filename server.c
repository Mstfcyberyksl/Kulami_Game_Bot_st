#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>

#define PORT 9000
#define ROWS 8
#define COLUMNS 8

FILE* general_data_file;
int server_fd;

// Signal handler for clean shutdown
void handle_signal(int sig) {
    printf("\nShutting down server...\n");
    close(server_fd);
    exit(0);
}

bool check_done_my(int board[ROWS][COLUMNS]) {
    bool check = true;
    int index = 0;
    
    general_data_file = fopen("game_data.txt", "r");
    if (general_data_file == NULL) {
        perror("Failed to open game_data.txt");
        return false;
    }
    
    char line[150];
    
    while (fgets(line, sizeof(line), general_data_file) != NULL) {
        check = true;
        index = 0;
        
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLUMNS; j++) {
                if (line[index] - '0' != board[i][j]) {
                    check = false;
                    break;
                }
                index += 2;
            }
            if (!check) {
                break;
            }
        }
        
        if (check) {
            fclose(general_data_file);
            return true;
        }
    }
    
    fclose(general_data_file);
    return false;
}

int main() {
    // Set up signal handling for clean shutdown
    signal(SIGINT, handle_signal);
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return 1;
    }
    
    // Set up address structure
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any interface
    address.sin_port = htons(PORT);
    
    // Bind socket to address and port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }
    
    // Listen for connections
    if (listen(server_fd, 5) < 0) {  // Allow queue of up to 5 connections
        perror("Listen failed");
        close(server_fd);
        return 1;
    }
    
    printf("Server listening on port %d...\n", PORT);
    printf("Press Ctrl+C to stop the server\n");
    
    while (1) {
        // Accept a connection
        int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        // Get client's IP for logging
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Client connected from %s:%d\n", client_ip, ntohs(address.sin_port));
        
        // Receive board data as flat array
        int board[ROWS][COLUMNS];
        int bytes_received = recv(new_socket, board, ROWS * COLUMNS * sizeof(int), 0);
        
        if (bytes_received < 0) {
            perror("Receive failed");
            close(new_socket);
            continue;
        } else if (bytes_received != ROWS * COLUMNS * sizeof(int)) {
            fprintf(stderr, "Incomplete data received: %d bytes\n", bytes_received);
            close(new_socket);
            continue;
        }
        
        printf("Board data received, checking solution...\n");
        
        // Check if board is a solution
        bool result = check_done_my(board);
        printf("Result: %s\n", result ? "Solution found" : "Not a solution");
        
        // Send result back to client
        if (send(new_socket, &result, sizeof(result), 0) < 0) {
            perror("Send failed");
        } else {
            printf("Result sent to client\n");
        }
        
        // Close client connection
        close(new_socket);
        printf("Client disconnected\n");
    }
    
    // This point should not be reached due to the infinite loop and signal handler
    close(server_fd);
    return 0;
}