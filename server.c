#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>

#define PORT 9000
#define rows 8
#define columns 8

FILE* general_data_file;

bool check_done_my(int** board){
    bool check = true;
    int index = 0;
    general_data_file = fopen("game_data.txt","r");
    char line[150];
    int** copy = (int**)malloc(rows * sizeof(int*));
    for(int i = 0;i < rows;i++){
        copy[i] = (int*)malloc(columns * sizeof(int));
        memcpy(copy[i],board[i],columns * sizeof(int));
    }
    while (fgets(line, sizeof(line), general_data_file) != NULL){
        check = true;
        index = 0;
        for(int i = 0;i < rows;i++){
            for(int j = 0;j < columns;j++){
                if (line[index] - '0' != copy[i][j]){
                    check = false;
                    break;
                }
                index += 2;
            }
            if(!check){
                break;
            }
        }
        if (check){
            for(int i = 0;i < rows;i++){
                free(copy[i]);
            }
            free(copy);
            fclose(general_data_file);
            return true;
        } 
    } 
    for(int i = 0;i < rows;i++){
        free(copy[i]);
    }
    free(copy);
    fclose(general_data_file);
    return false;
}

int main() {
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);

        return 1;
    }

    
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);
    
    while (1) {
        int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected\n");

        int** board = (int**)malloc(8 * sizeof(int*));
        for (int i = 0; i < 8; i++) {
            board[i] = (int*)malloc(8 * sizeof(int));
        }

        // Receive board data
        recv(new_socket, *board, 8 * 8 * sizeof(int), 0);

        bool result = check_done_my(board);
        send(new_socket, &result, sizeof(result), 0);

        close(new_socket);
    }

    close(server_fd);

    return 0;
}