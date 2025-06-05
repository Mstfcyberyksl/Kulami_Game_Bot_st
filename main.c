#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <limits.h>
#include "kulami_config.h"

#define calcfuncsize 6
#define ROWS 8
#define COLUMNS 8
#define directionsize 28
#define framecount 17
#define data_length 8
#define PORT 9000
#define BOARD_POOL_SIZE 400
#define ROW_ARRAY_SIZE 2000
#define DATA2_POOL_SIZE 1000
#define DATA_POOL_SIZE 500
#define NODE_POOL_SIZE 100
#define INT_ARRAY_POOL_SIZE 300
#define MAX_MOVE_BUFFER 64     // Maximum moves to consider in a turn
#define DATA_SIZE_CLIENT (ROWS * COLUMNS + 7)  // Board + extra parameters
int* best_place(int x, int y,int step, int lx, int ly, int userframe, int pcframe);
// Network communication helper functions
static ssize_t send_all(int sockfd, const void* buf, size_t len) {
    size_t total_sent = 0;
    const char* ptr = (const char*)buf;
    
    while (total_sent < len) {
        ssize_t sent = send(sockfd, ptr + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            return -1;  // Error or connection closed
        }
        total_sent += sent;
    }
    return total_sent;
}

static ssize_t recv_all(int sockfd, void* buf, size_t len) {
    size_t total_received = 0;
    char* ptr = (char*)buf;
    
    while (total_received < len) {
        ssize_t received = recv(sockfd, ptr + total_received, len - total_received, 0);
        if (received <= 0) {
            return -1;  // Error or connection closed
        }
        total_received += received;
    }
    return total_received;
}

static void free_local_board(int** board, int allocated_rows) {
    if (board) {
        for (int i = 0; i < allocated_rows; i++) {
            free(board[i]);
        }
        free(board);
    }
}

// Inline helper functions for better performance
static inline bool is_valid_position(int x, int y) {
    return x >= 0 && x < ROWS && y >= 0 && y < COLUMNS;
}

static inline int calculate_position_score(int x, int y) {
    // Favor center positions
    return -(abs(x - ROWS/2) + abs(y - COLUMNS/2));
}


int area = 0, userframe = -1, pcframe = -1;
int genstep = -1, validlength = -1;

int** board2;
FILE* file;
FILE* inputfile;
FILE* outputfile;
FILE* savefile;
FILE* general_data_file;

int marble_result,oneslen = 0;

int** ones;

const int directions[directionsize][2] = {
    {0, -1}, {0, -2}, {0, -3}, {0, -4}, {0, -5}, {0, -6}, {0, -7},
    {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7},
    {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0},
    {-1, 0}, {-2, 0}, {-3, 0}, {-4, 0}, {-5, 0}, {-6, 0}, {-7, 0}
};

const int frames[framecount][13] = {{4,0,0,0,1,1,0,1,1,-1,-1,-1,-1},
                        {6,0,2,0,3,0,4,1,2,1,3,1,4},
                        {6,0,5,0,6,1,5,1,6,2,5,2,6},
                        {3,0,7,1,7,2,7,-1,-1,-1,-1,-1,-1},
                        {2,2,0,3,0,-1,-1,-1,-1,-1,-1,-1,-1},
                        {4,2,1,2,2,3,1,3,2,-1,-1,-1,-1},
                        {2,2,3,2,4,-1,-1,-1,-1,-1,-1,-1,-1},
                        {3,4,0,4,1,4,2,-1,-1,-1,-1,-1,-1},
                        {6,3,3,3,4,3,5,4,3,4,4,4,5},
                        {2,3,6,3,7,-1,-1,-1,-1,-1,-1,-1,-1},
                        {4,4,6,4,7,5,6,5,7,-1,-1,-1,-1},
                        {6,5,0,5,1,6,0,6,1,7,0,7,1},
                        {2,5,2,6,2,-1,-1,-1,-1,-1,-1,-1,-1},
                        {4,5,3,5,4,6,3,6,4,-1,-1,-1,-1},
                        {3,5,5,6,5,7,5,-1,-1,-1,-1,-1,-1},
                        {3,7,2,7,3,7,4,-1,-1,-1,-1,-1,-1},
                        {4,6,6,6,7,7,6,7,7,-1,-1,-1,-1}};
const int places_45[7][2] = {
        {7,0},{6,0},{5,0},{4,0},{7,3},{7,2},{7,1}
    };

const int places_135[7][2] = {
        {7,4},{6,7},{5,7},{4,7},{7,5},{7,6},{7,7}
    };

typedef struct Node {
    int frame;
} Node;

typedef struct {
    int color;
    int** board;
    int* horizontal_info;
    int* vertical_info;
}Data2;

Node* newnode[ROWS][COLUMNS];
typedef struct {
    int* data1;
    //int x;
    //int y;
    //int step;
    //int not_x;
    //int not_y;
    //int color;
    //int alpha;
    //int beta;
    bool ret;
    bool is_max;
    int** board;
}Data;

Node* newnode[ROWS][COLUMNS];

typedef struct {
    int** boards[BOARD_POOL_SIZE];
    bool used[BOARD_POOL_SIZE];
    int initialized;
} BoardPool;

BoardPool board_pool = {0};

typedef struct{
    int* arrays[ROW_ARRAY_SIZE];
    bool used[ROW_ARRAY_SIZE];
    int initialized;
} Row_Array_Pool;

Row_Array_Pool row_array_pool = {0};

void init_board_pool() {
    if (board_pool.initialized) return;

    for (int i = 0; i < BOARD_POOL_SIZE; i++) {
        board_pool.boards[i] = (int**)malloc(ROWS * sizeof(int*));
        for (int j = 0; j < ROWS; j++) {
            board_pool.boards[i][j] = (int*)malloc(COLUMNS * sizeof(int));
        }
        board_pool.used[i] = false;
    }
    board_pool.initialized = 1;
}

void init_row_array_pool(){
    if (row_array_pool.initialized) return;
    for(int i = 0;i < ROW_ARRAY_SIZE;i++){
        row_array_pool.arrays[i] = (int*)malloc(ROWS * sizeof(int));
        row_array_pool.used[i] = false;
    }
    row_array_pool.initialized = 1;
}

int** get_board_from_pool() {
    if(!board_pool.initialized) {
        init_board_pool();
    }
    for (int i = 0; i < BOARD_POOL_SIZE; i++) {
        if (!board_pool.used[i]) {
            board_pool.used[i] = true;
            return board_pool.boards[i];
        }
    }
    // Pool exhausted, fallback to malloc
    int** board = (int**)malloc(ROWS * sizeof(int*));
    if (!board) {
        perror("Failed to allocate board from pool");
        return NULL; // Handle allocation failure
    }
    for (int i = 0; i < ROWS; i++) {
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
        if (!board[i]) {
            perror("Failed to allocate row in board from pool");
            for (int j = 0; j < i; j++) {
                free(board[j]);
            }
            free(board);
            return NULL; // Handle allocation failure
        }
    }
    return board;
}

int* get_row_array_from_pool(){
    if(!row_array_pool.initialized){
        init_row_array_pool();
    }
    for(int i = 0;i < ROW_ARRAY_SIZE;i++){
        if(!row_array_pool.used[i]){
            row_array_pool.used[i] = true;
            return row_array_pool.arrays[i];
        }
    }
    int* array = (int*)malloc(ROWS * sizeof(int));
    if (!array) {
        perror("Failed to allocate row array from pool");
        return NULL; // Handle allocation failure
    }
    return array;
}

void return_board_to_pool(int** board) {
    for (int i = 0; i < BOARD_POOL_SIZE; i++) {
        if (board_pool.boards[i] == board) {
            board_pool.used[i] = false;
            return;
        }
    }
    // Not from pool, free normally
    for (int i = 0; i < ROWS; i++) {
        free(board[i]);
    }
    free(board);
}

void return_row_array_to_pool(int* array) {
    for (int i = 0; i < ROW_ARRAY_SIZE; i++) {
        if (row_array_pool.arrays[i] == array) {
            row_array_pool.used[i] = false;
            return;
        }
    }
    // Not from pool, free normally
    free(array);
}
void freedata2(Data2* data){
    return_board_to_pool(data->board);
    return_row_array_to_pool(data->horizontal_info);
    return_row_array_to_pool(data->vertical_info);
}

void freedata(Data* data){
    return_board_to_pool(data->board);
    free(data->data1);
}

int horizontal_points(void *arg){
    int length, pc = 0,user = 0, result;

    Data2* data = (Data2*)arg;

    // Use direct board access instead of copying
    int** board = data->board;

    for (int i = 0;i < ROWS;i++){
        if(data->horizontal_info[i] >= 5){
            length = 1;
            for (int j = 1;j < COLUMNS;j++){
                if (board[i][j] != 0 && board[i][j-1] == board[i][j]){
                    length++;
                }
                else{
                    if (length >= 5){
                        if (board[i][j-1] == 1){
                            user += length;
                        }
                        else if (board[i][j-1] == 2){
                            pc += length;
                        }
                    }
                    length = 1;
                }
            }
            if (length >= 5){
                if (board[i][7] == 1){
                    user += length;
                }
                else{
                    pc += length;
                }
            }
        }
    }
    result = pc - user;
    return result;
}
int vertical_points(void *arg){
    int length,pc = 0,user = 0,result;

    Data2* data = (Data2*)arg;
    int** board = data->board;


    for (int j = 0;j < ROWS;j++){
        if(data->vertical_info[j] >= 5){
            length = 1;
            for (int i = 1;i < COLUMNS;i++){
                if (board[i][j] != 0 && board[i-1][j] == board[i][j]){
                    length++;
                }
                else{
                    if (length >= 5){
                        if (board[i-1][j] == 1){
                            user += length;
                        }
                        else if (board[i-1][j] == 2){
                            pc += length;
                        }
                    }
                    length = 1;
                }
            }
            if (length >= 5){
                if (board[7][j] == 1){
                    user += length;
                }
                else{
                    pc += length;
                }
            }
        }
    }

    result = pc - user;
    return result;
}
int diagonal_points_45(void *arg){
    int length, pc = 0, user = 0, x, y, result;

    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(ROWS * sizeof(int*));
    for (int i = 0;i < ROWS;i++){
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(board[i],data->board[i],COLUMNS * sizeof(int));
    }

    for(int i = 0;i < 7;i++){
        x = places_45[i][0];
        y = places_45[i][1];
        length = 1;
        while (x < ROWS && y < COLUMNS && x > -1 && y > -1){
            if (x+1 < ROWS && x+1 > -1 && y-1 < COLUMNS && y-1 > -1 && board[x][y] != 0 && board[x][y] == board[x+1][y-1]){
                length++;
            }
            else{
                if (length >= 5){
                    if (x+1 < ROWS && x+1 > -1 && y-1 < COLUMNS && y-1 > -1 && board[x+1][y-1] == 2){
                        pc += length;
                    }
                    else{
                        user += length;
                    }
                }
                length = 1;
            }
            x--;
            y++;
        }
        if (length >= 5){
            x++;
            y--;
            if (board[x][y] == 1){
                user += length;
            }
            else{
                pc += length;
            }
        }
    }

    result = pc - user;
    for (int i = 0;i < ROWS;i++){
        free(board[i]);
    }
    free(board);
    return result;
}
int diagonal_points_135(void *arg){
    int length, pc = 0, user = 0,x,y, result;

    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(ROWS * sizeof(int*));
    for (int i = 0;i < ROWS;i++){
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(board[i],data->board[i],COLUMNS * sizeof(int));
    }

    for(int i = 0;i < 7;i++){
        x = places_135[i][0];
        y = places_135[i][1];
        length = 0;
        while (x > -1 && y > -1){
            if (x+1 < ROWS && x+1 > -1 && y+1 < COLUMNS && y+1 > -1 && board[x][y] != 0 && board[x][y] == board[x+1][y+1]){
                length++;
            }
            else{
                if (length >= 5){
                    if (board[x+1][y+1] == 2){
                        pc += length;
                    }
                    else{
                        user += length;
                    }
                }
                length = 1;
            }
            x--;
            y--;
        }
        if (length >= 5){
            x++;
            y++;
            if (board[x][y] == 1){
                user += length;
            }
            else{
                pc += length;
            }
        }
    }

    result = pc - user;
    for (int i = 0;i < ROWS;i++){
        free(board[i]);
    }
    free(board);
    return result;
}
int dfs(int i,int j,int** board,int color){
    if (i < 0 || i == ROWS || j < 0 || j == COLUMNS || board[i][j] != color) {
        return 0;
    }
    board[i][j] = 0;
    return 1 + dfs(i+1,j,board,color) + dfs(i-1,j,board,color) + dfs(i,j+1,board,color) + dfs(i,j-1,board,color);
}
int marble_area_points(void *arg){
    Data2* data = (Data2*)arg;
    int user = 0, pc = 0, temp, result;
    int** board = (int**)malloc(ROWS * sizeof(int*));
    for (int i = 0;i < ROWS;i++){
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(board[i],data->board[i],COLUMNS * sizeof(int));
    }

    for(int i = 0;i < ROWS;i++){
        for(int j = 0;j < COLUMNS;j++){
            if(board[i][j] == 1){
                temp = dfs(i,j,board,1);
                if (temp > user){
                    user = temp;
                }
            }else if (board[i][j] == 2){
                temp = dfs(i,j,board,2);
                if (temp > pc){
                    pc = temp;
                }
            }
        }
    }

    result = pc - user;

    for (int i = 0;i < ROWS;i++){
        free(board[i]);
    }
    free(board);
    return result;
}
int place_area_points(void *arg){
    int pc = 0,user = 0,length = 0, result;
    Data2* data = (Data2*)arg;
    int** board = data->board;

    for (int i = 0;i < 17;i++){
        pc = 0;
        user = 0;
        for (int j = 1; j < 2 * frames[i][0]; j += 2){
            if (board[frames[i][j]][frames[i][j+1]] == 2){
                pc++;
            }
            else if (board[frames[i][j]][frames[i][j+1]] == 1){
                user++;
            }
        }
        if (pc > user){
            length += frames[i][0];
        }
        else if (user > pc){
            length -= frames[i][0];
        }
    }
    result = length;
    return result;
}
Data2* copydata2(Data2* data){
    Data2* copy;
    copy = (Data2*)malloc(sizeof(Data2));
    copy->board = (int**)malloc(ROWS * sizeof(int*));
    for (int i = 0; i < ROWS; i++) {
        copy->board[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(copy->board[i], data->board[i], COLUMNS * sizeof(int));
    }
    copy->horizontal_info = (int*)malloc(ROWS * sizeof(int));
    copy->vertical_info = (int*)malloc(COLUMNS * sizeof(int));
    memcpy(copy->horizontal_info,data->horizontal_info,ROWS * sizeof(int));
    memcpy(copy->vertical_info,data->vertical_info,COLUMNS * sizeof(int));
    return copy;
}


int calculate(int** board){
    Data2* data = (Data2*)malloc(sizeof(Data2));
    data->board = get_board_from_pool();
    for (int i = 0;i < ROWS;i++){
        memcpy(data->board[i],board[i],COLUMNS * sizeof(int));
    }
    data->horizontal_info = get_row_array_from_pool();
    data->vertical_info = get_row_array_from_pool();
    for(int i = 0;i < ROWS;i++){
        data->horizontal_info[i] = 0;
        data->vertical_info[i] = 0;
    }
    for(int i = 0;i < ROWS;i++){
        for(int j = 0;j < COLUMNS;j++){
            if(data->board[i][j] != 0){
                data->horizontal_info[i]++;
            }
            if(data->board[j][i] != 0){
                data->vertical_info[i]++;
            }
        }
    }

    int sum = 0;
    sum += horizontal_points((void*)data);
    sum += vertical_points((void*)data);
    sum += diagonal_points_45((void*)data);
    sum += diagonal_points_135((void*)data);
    sum += place_area_points((void*)data);
    sum += marble_area_points((void*)data);

    freedata2(data);
    free(data);
    return sum;
}

int which(int x, int y){
    for (int i = 0;i < 17;i++){
        for (int j = 1;j < 2 * frames[i][0];j += 2){
            if (frames[i][j] == x && frames[i][j+1] == y){
                return i;
            }
        }
    }
    return -1;
}

int generate_data() {
    int client_fd = -1;
    int* best_place_res = NULL;
    int** local_board = NULL;
    int result_buffer[DATA_SIZE_CLIENT]; // Use a fixed-size buffer

    // Allocate local_board
    local_board = (int**)get_board_from_pool();
    if (!local_board) {
        perror("Failed to allocate local_board (ROWS)");
        return -1;
    }
    

    // --- First Connection: Get latest state (Mode 1) ---
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed (Mode 1)");
        free_local_board(local_board, ROWS);
        return -1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    // IP ADDRESS   
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(client_fd);
        free_local_board(local_board, ROWS);
        return -1;
    }

    printf("Attempting to connect to server (Mode 1)...\n");
    if (connect(client_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed (Mode 1)");
        close(client_fd);
        free_local_board(local_board, ROWS);
        return -1;
    }
    printf("Connected to server (Mode 1)\n");

    int mode = 1;
    int net_mode = htonl(mode);
    if (send_all(client_fd, &net_mode, sizeof(net_mode)) < 0) {
        fprintf(stderr, "Send mode 1 failed\n");
        close(client_fd);
        free_local_board(local_board, ROWS);
        return -1;
    }
    printf("MODE 1 sent to server\n");

    // Receive result (could be full array or single error int)
    ssize_t received_bytes = recv(client_fd, result_buffer, sizeof(result_buffer), 0);

    if (received_bytes < 0) {
        perror("Receive failed (Mode 1)");
        close(client_fd);
        free_local_board(local_board, ROWS);
        return -1;
    } else if (received_bytes == 0) {
        fprintf(stderr, "Server closed connection unexpectedly (Mode 1 receive)\n");
        close(client_fd);
        free_local_board(local_board, ROWS);
        return -1;
    } else if (received_bytes == sizeof(int)) {
        // Assume it's a single integer error/status code from the server
        int status_code = ntohl(result_buffer[0]);
        printf("Received status/error code from server (Mode 1): %d\n", status_code);
        if (status_code < 0) { // Typically negative for errors
             fprintf(stderr, "Server indicated an error or no data available.\n");
        }
        // Cannot proceed to game logic if we don't have full board
        close(client_fd);
        free_local_board(local_board, ROWS);
        return -1; // Or handle differently based on status_code
    } else if (received_bytes == sizeof(result_buffer)) {
        printf("Full game state received from server.\n");
        // Full array received, convert each element from network to host byte order
        for (int i = 0; i < DATA_SIZE_CLIENT; i++) {
            result_buffer[i] = ntohl(result_buffer[i]);
        }

        // Populate local_board from the first ROWS * COLUMNS elements
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLUMNS; j++) {
                local_board[i][j] = result_buffer[COLUMNS * i + j];
            }
        }

        // Call game logic
        printf("Calling best_place logic...\n");
        best_place_res = best_place(result_buffer[DATA_SIZE_CLIENT - 7],
                                result_buffer[DATA_SIZE_CLIENT - 6],
                                    result_buffer[DATA_SIZE_CLIENT - 5],
                                   result_buffer[DATA_SIZE_CLIENT - 4],
                                   result_buffer[DATA_SIZE_CLIENT - 3],
                                   result_buffer[DATA_SIZE_CLIENT - 2],
                                   result_buffer[DATA_SIZE_CLIENT - 1]);
        if (!best_place_res) {
            fprintf(stderr, "best_place function failed or returned NULL.\n");
            close(client_fd);
            free_local_board(local_board, ROWS);
            return -1;
        }
    } else {
        fprintf(stderr, "Received unexpected number of bytes (Mode 1): %zd\n", received_bytes);
        close(client_fd);
        free_local_board(local_board, ROWS);
        return -1;
    }
    close(client_fd); // Close first connection

    // If best_place_res is NULL here, it means we didn't get full data or best_place failed.
    // The previous conditional block should have handled this. This is a safeguard.
    if (!best_place_res) {
        fprintf(stderr, "Cannot proceed to Mode 2: best_place result is not available.\n");
        free_local_board(local_board, ROWS);
        // best_place_res is already NULL, no need to free.
        return -1;
    }

    // --- Second Connection: Send updated state (Mode 2) ---
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed (Mode 2)");
        free_local_board(local_board, ROWS);
        free(best_place_res);
        return -1;
    }

    printf("Attempting to connect to server (Mode 2)...\n");
    if (connect(client_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed (Mode 2)");
        close(client_fd);
        free_local_board(local_board, ROWS);
        free(best_place_res);
        return -1;
    }
    printf("Reconnected to server (Mode 2)\n");

    mode = 2;
    net_mode = htonl(mode);
    if (send_all(client_fd, &net_mode, sizeof(net_mode)) < 0) {
        fprintf(stderr, "Send mode 2 failed\n");
        close(client_fd);
        free_local_board(local_board, ROWS);
        free(best_place_res);
        return -1;
    }
    printf("MODE 2 sent to server\n");

    // Prepare the result_buffer to send:
    // 1. Copy modified local_board back to the start of result_buffer
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            result_buffer[COLUMNS * i + j] = local_board[i][j];
        }
    }
    // 2. Update the extra parameters based on game logic (example logic from original)
    // Assuming original result_buffer[64] through result_buffer[68] were used as input
    // and now we update some of them with best_place_res.
    // The original client's update logic:
    // result[67] = result[64]; // This was using original result[64]
    // result[68] = result[65]; // This was using original result[65]
    // For this example, let's assume result_buffer still holds the necessary values for elements 66, 67, 68
    // And elements 64, 65 are updated by best_place_res.
    // indices: DATA_SIZE_CLIENT-5, DATA_SIZE_CLIENT-4, ..., DATA_SIZE_CLIENT-1

    // This part needs to be correct according to your game logic.
    // Example: Keep original value for result_buffer[DATA_SIZE_CLIENT-1] (orig index 68)
    // Keep original value for result_buffer[DATA_SIZE_CLIENT-2] (orig index 67)
    // Keep original value for result_buffer[DATA_SIZE_CLIENT-3] (orig index 66)
    // Update DATA_SIZE_CLIENT-5 (orig 64) and DATA_SIZE_CLIENT-4 (orig 65)
    result_buffer[DATA_SIZE_CLIENT - 5] = best_place_res[0]; // Example: best_place_res[0] -> result_buffer[64]
    result_buffer[DATA_SIZE_CLIENT - 4] = best_place_res[1]; // Example: best_place_res[1] -> result_buffer[65]
    // The other EXTRA_CLIENT fields (e.g., result_buffer[66,67,68]) would retain their values
    // from the initial Mode 1 fetch if not explicitly changed here.

    printf("Preparing to send updated game state...\n");
    // Convert the entire result_buffer to network byte order before sending
    for (int i = 0; i < DATA_SIZE_CLIENT; i++) {
        result_buffer[i] = htonl(result_buffer[i]);
    }

    if (send_all(client_fd, result_buffer, sizeof(result_buffer)) < 0) {
        fprintf(stderr, "Send updated game state failed\n");
        close(client_fd);
        free_local_board(local_board, ROWS);
        free(best_place_res);
        return -1;
    }
    printf("Updated game state sent to server.\n");

    // Receive confirmation from server
    int confirmation_code_net;
    if (recv_all(client_fd, &confirmation_code_net, sizeof(confirmation_code_net)) < 0) {
        fprintf(stderr, "Failed to receive confirmation (Mode 2)\n");
        // Still, we attempted the operation, so clean up and decide on return.
    } else {
        int confirmation_code_host = ntohl(confirmation_code_net);
        printf("Server confirmation (Mode 2): %d\n", confirmation_code_host);
        if (confirmation_code_host != 1) {
             fprintf(stderr, "Server indicated failure to store new state.\n");
             // Optionally change return status based on this
        }
    }

    close(client_fd);
    free_local_board(local_board, ROWS);
    free(best_place_res);

    printf("generate_data completed successfully.\n");
    return 0; // Success
}

static inline int piece_count(int** board){
    int count = 0;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            if (board[i][j] != 0) {
                count++;
            }
        }
    }
    return count;
}

// Enhanced move evaluation for better ordering
static inline int evaluate_move_score(int x, int y, int** board) {
    int score = 0;
    
    // Center positions are better
    score += 20 - (abs(x - ROWS/2) + abs(y - COLUMNS/2)) * 2;
    
    // Count adjacent pieces (connectivity bonus)
    int adjacent = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (is_valid_position(nx, ny) && board[nx][ny] != 0) {
                adjacent++;
            }
        }
    }
    score += adjacent * 15;
    
    // Edge and corner penalties
    if (x == 0 || x == ROWS-1) score -= 5;
    if (y == 0 || y == COLUMNS-1) score -= 5;
    if ((x == 0 || x == ROWS-1) && (y == 0 || y == COLUMNS-1)) score -= 10;
    
    return score;
}
int moves[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
// Helper function for quicksort
void swap_moves(int directions_local[][2], int direction_scores[], int i, int j) {
    int temp_score = direction_scores[i];
    direction_scores[i] = direction_scores[j];
    direction_scores[j] = temp_score;
    
    int temp_dir0 = directions_local[i][0];
    int temp_dir1 = directions_local[i][1];
    directions_local[i][0] = directions_local[j][0];
    directions_local[i][1] = directions_local[j][1];
    directions_local[j][0] = temp_dir0;
    directions_local[j][1] = temp_dir1;
}

int partition_moves(int directions_local[][2], int direction_scores[], int low, int high) {
    int pivot = direction_scores[high];
    int i = low - 1;
    
    for (int j = low; j <= high - 1; j++) {
        if (direction_scores[j] > pivot) {  // Sort in descending order
            i++;
            swap_moves(directions_local, direction_scores, i, j);
        }
    }
    swap_moves(directions_local, direction_scores, i + 1, high);
    return i + 1;
}

void quicksort_moves(int directions_local[][2], int direction_scores[], int low, int high) {
    if (low < high) {
        int pi = partition_moves(directions_local, direction_scores, low, high);
        quicksort_moves(directions_local, direction_scores, low, pi - 1);
        quicksort_moves(directions_local, direction_scores, pi + 1, high);
    }
}

int surrounding_piece_count(int** board, int x, int y){
    int result = 0;

    for(int i = 0;i < 8;i++){
        if(x+moves[i][0] < 8 && x+moves[i][0] > -1 && y+moves[i][1] < 8 && y+moves[i][1] > -1 && board[x+moves[i][0]][y+moves[i][1]] != 0){
            result++;
        }
    }
    return result;
}
int* search(void *arg){
    Data* data = (Data*)arg;
    if (data->data1[2] == 0){
        int* result = (int*)malloc(sizeof(int));
        *result = calculate(data->board);
        return result;
    }

    // Move ordering optimization for better alpha-beta pruning

    int info1, info2, maximum, move_count = 0;
    int directions_local[directionsize][2];
    int direction_scores[directionsize];

    if (data->is_max){
        maximum = -1000000;
    }else{
        maximum = 1000000;
    }
    Data** datas = (Data**)malloc(directionsize * sizeof(Data*));

    int* result;
    if(data->ret){
        result = (int*)malloc(2 * sizeof(int));
        result[0] = -1;
        result[1] = -1;
    }else{
        result = (int*)malloc(sizeof(int));
        *result = -1;
    }

    if (data->data1[3] > -1 && data->data1[4] > -1){
        info1 = newnode[data->data1[3]][data->data1[4]]->frame;
    }else{
        info1 = -1;
    }
    info2 = newnode[data->data1[0]][data->data1[1]]->frame;
    data->board[data->data1[0]][data->data1[1]] = data->data1[5];

    if (data->data1[5] == 2){
        data->data1[5] = 1;
    }else if (data->data1[5] == 1){
        data->data1[5] = 2;
    }else{
        printf("COLOR ERROR %d",data->data1[5]);
    }

    bool piece_check = piece_count(data->board) < 56;
    bool got_into = false;

    for(int k = 0;k < directionsize;k++){
        //  result[0] + x 
        if ((data->data1[0] + directions[k][0] != data->data1[3] ||
            data->data1[1] + directions[k][1] != data->data1[4]) &&
            data->data1[0] + directions[k][0] < ROWS &&
            data->data1[0] + directions[k][0] > -1 &&
            data->data1[1] + directions[k][1] < COLUMNS &&
            data->data1[1] + directions[k][1] > -1 &&
            data->board[data->data1[0] + directions[k][0]][data->data1[1] + directions[k][1]] == 0 &&
            newnode[data->data1[0] + directions[k][0]][data->data1[1] + directions[k][1]]->frame != info1 &&
            newnode[data->data1[0] + directions[k][0]][data->data1[1] + directions[k][1]]->frame != info2 &&
            piece_check){
                directions_local[move_count][0] = directions[k][0];
                directions_local[move_count][1] = directions[k][1];
                direction_scores[move_count] = -1 * abs(data->data1[0] + directions[k][0] - ROWS/2) - abs(data->data1[1] + directions[k][1] - COLUMNS/2) + surrounding_piece_count(data->board,data->data1[0] + directions[k][0], data->data1[1] + directions[k][1]);
                move_count++;
            }
    }
    // Use quicksort instead of selection sort for better performance
    if (move_count > 1) {
        quicksort_moves(directions_local, direction_scores, 0, move_count - 1);
    }

    for (int k = 0;k < move_count;k++){
            got_into = true;
            data->board[data->data1[0] + directions_local[k][0]][data->data1[1] + directions_local[k][1]] = data->data1[5];

            datas[k] = (Data*)malloc(sizeof(Data));
            datas[k]->data1 = (int*)malloc(data_length * sizeof(int));
            datas[k]->data1[0] = data->data1[0] + directions_local[k][0];
            datas[k]->data1[1] = data->data1[1] + directions_local[k][1];
            datas[k]->data1[2] = data->data1[2] - 1;
            datas[k]->data1[3] = data->data1[0];
            datas[k]->data1[4] = data->data1[1];
            datas[k]->data1[5] = data->data1[5];
            datas[k]->data1[6] = data->data1[6];
            datas[k]->data1[7] = data->data1[7];
            datas[k]->is_max = !data->is_max;
            datas[k]->ret = false;
            datas[k]->board = get_board_from_pool();
            for (int i = 0; i < ROWS; i++) {
                memcpy(datas[k]->board[i], data->board[i], COLUMNS * sizeof(int));
            }

            int* temppp = search((void*)datas[k]);
            if (data->is_max){
                if (*temppp > maximum){
                    maximum = *temppp;
                    if(data->ret){
                        result[0] = data->data1[0] + directions_local[k][0];
                        result[1] = data->data1[1] + directions_local[k][1];
                    }
                }
                if(maximum > data->data1[6] ){
                    data->data1[6] = maximum;
                }

                if (data->data1[6] >= data->data1[7]){
                    freedata(datas[k]);
                    free(datas[k]);
                    free(temppp);
                    data->board[data->data1[0] + directions_local[k][0]][data->data1[1] + directions_local[k][1]] = 0;
                    break;
                }
            }else{
                if (*temppp < maximum){
                    maximum = *temppp;
                    if(data->ret){
                        result[0] = data->data1[0] + directions_local[k][0];
                        result[1] = data->data1[1] + directions_local[k][1];
                    }
                }
                if(maximum < data->data1[7] ){
                    data->data1[7] = maximum;
                }
                if (data->data1[7] <= data->data1[6]){
                    freedata(datas[k]);
                    free(datas[k]);
                    free(temppp);
                    data->board[data->data1[0] + directions_local[k][0]][data->data1[1] + directions_local[k][1]] = 0;
                    break;
                }
            }
            freedata(datas[k]);
            free(datas[k]);
            free(temppp);
            data->board[data->data1[0] + directions_local[k][0]][data->data1[1] + directions_local[k][1]] = 0;

    }

    free(datas);
    if (data->ret){
        return result;
    }
    if(!got_into){
        maximum = calculate(data->board);
    }
    *result = maximum;
    return result;
}

int* best_place(int x, int y,int step, int lx, int ly,int userframe ,int pcframe){// username pc frame ekle
    int i, j;
    printf("START SEARCH\n");
    printf("Debug: Starting search at position (%d,%d) with step %d, lx %d, ly %d, userframe %d, pcframe %d\n", x, y, step, lx, ly, userframe, pcframe);
    board2[x][y] = 1;

    genstep = step;
    i = newnode[x][y]->frame;
    if (i != userframe && i != pcframe){
        userframe = i;
        printf("Debug: Updated userframe to %d\n", userframe);
    }
    else{
        printf("FRAME ERROR: frame conflict at position (%d,%d) - frame=%d already used (userframe=%d, pcframe=%d)\n", x, y, i, userframe, pcframe);
        int* temp;
        temp = (int*)malloc(2 * sizeof(int));
        temp[0] = -1;
        temp[1] = -1;
        return temp;
    }
    int* temp;
    Data data;
    data.data1 = (int*)malloc(data_length * sizeof(int));
    data.data1[0] = x;
    data.data1[1] = y;
    data.data1[2] = step;
    data.data1[3] = lx;
    data.data1[4] = ly;
    data.data1[5] = 1;
    data.data1[6] = -999999999;
    data.data1[7] = 999999999;
    data.is_max = true;
    data.ret = true;
    int** board = get_board_from_pool();
    for (i = 0;i < ROWS;i++){
        memcpy(board[i],board2[i], COLUMNS * sizeof(int));
    }

    data.board = board;

    file = fopen("data.csv","a");
    fprintf(file,"%d, %d, %d, %d, ",x,y,lx,ly);
    for(int i = 0;i < ROWS;i++){
        for(int j = 0;j < COLUMNS;j++){
            fprintf(file,"%d, ",board2[i][j]);
        }
    }

    temp = search((void*)&data);
    fprintf(file,"%d, %d\n",temp[0],temp[1]);
    fclose(file);

    // Clean up memory
    return_board_to_pool(board);
    free(data.data1);

    board2[temp[0]][temp[1]] = 2;

    j = newnode[temp[0]][temp[1]]->frame;
    if (j != userframe && j != pcframe){
        pcframe = j;
    }
    else{
        printf("PC FRAME ERROR\n");
    }
    return temp;
}

int main(){
    // Initialize memory pools for better performance
    init_board_pool();
    
    
    
    board2 = get_board_from_pool();
    // make everywhere empty
    for (int i = 0;i < ROWS; i++){
        for (int j = 0;j < COLUMNS;j++){
            board2[i][j] = 0;
        }
    }
    ones = (int**)malloc(sizeof(int*));

    for (int i = 0;i < ROWS;i++){
        for(int j = 0;j < COLUMNS;j++){
            newnode[i][j] = (Node*)malloc(sizeof(Node));
            newnode[i][j]->frame = which(i,j);
        }
    }
    return 0; // ADD FINDVALID FUNCTION, FIND THE VALID MOVES AND RUN THEM
}
// gcc -fPIC -shared -o kulami_game.so main.c -Ofast -march=native -flto -funroll-loops -fomit-frame-pointer -mtune=native -finline-functions