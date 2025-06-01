#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "kulami_config.h"

#define calcfuncsize 6
#define ROWS 8
#define COLUMNS 8
#define directionsize 28
#define framecount 17
#define data_length 8
#define PORT 9000
#define BOARD_POOL_SIZE 100

// Inline helper functions for better performance
static inline bool is_valid_position(int x, int y) {
    return x >= 0 && x < ROWS && y >= 0 && y < COLUMNS;
}

static inline int calculate_position_score(int x, int y) {
    // Favor center positions
    return -(abs(x - ROWS/2) + abs(y - COLUMNS/2));
}

// Memory pool for board allocations
typedef struct {
    int** boards[BOARD_POOL_SIZE];
    bool used[BOARD_POOL_SIZE];
    int initialized;
} BoardPool;

BoardPool board_pool = {0};

// Connection pooling for socket optimization
static int cached_socket = -1;
static struct sockaddr_in cached_server_addr;
static bool socket_initialized = false;

// Move ordering: prioritize center positions and high-scoring moves
static inline int evaluate_move_priority(int x, int y, int** board) {
    int score = 0;
    
    // Favor center positions (better connectivity)
    score += 10 - (abs(x - ROWS/2) + abs(y - COLUMNS/2));
    
    // Check adjacent occupied positions (better for scoring)
    int adjacent_count = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (is_valid_position(nx, ny) && board[nx][ny] != 0) {
                adjacent_count++;
            }
        }
    }
    score += adjacent_count * 5;
    
    return score;
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
typedef struct {
    int color;
    int** board;
    int* horizontal_info;
    int* vertical_info;
}Data2;

typedef struct Node {
    int frame;
}Node;
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

// Memory pool functions for better performance
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

int** get_board_from_pool() {
    for (int i = 0; i < BOARD_POOL_SIZE; i++) {
        if (!board_pool.used[i]) {
            board_pool.used[i] = true;
            return board_pool.boards[i];
        }
    }
    // Pool exhausted, fallback to malloc
    int** board = (int**)malloc(ROWS * sizeof(int*));
    for (int i = 0; i < ROWS; i++) {
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
    }
    return board;
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

void freedata2(Data2* data){
    for (int i = 0; i < ROWS; i++) {
        free(data->board[i]);
    }
    free(data->board);
    free(data->horizontal_info);
    free(data->vertical_info);
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
    copy->color = data->color;
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


int calculate(int color, int** board){
    Data2* data = (Data2*)malloc(sizeof(Data2));
    data->color = color;
    data->board = (int**)malloc(ROWS * sizeof(int*));
    for (int i = 0;i < ROWS;i++){
        data->board[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(data->board[i],board[i],COLUMNS * sizeof(int));
    }
    data->horizontal_info = (int*)malloc(ROWS * sizeof(int));
    data->vertical_info = (int*)malloc(COLUMNS * sizeof(int));
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
    Data2** parray = (Data2**)malloc(calcfuncsize * sizeof(Data2*));
    for(int i = 0;i < calcfuncsize;i++){
        parray[i] = copydata2(data);
    }

    int sum = horizontal_points((void*)parray[0]) + vertical_points((void*)parray[1]) + diagonal_points_45((void*)parray[2]) + diagonal_points_135((void*)parray[3]) + place_area_points((void*)parray[4]) + marble_area_points((void*)parray[5]);

    for(int i = 0;i < calcfuncsize; i++){
        freedata2(parray[i]);
        free(parray[i]);
    }
    free(parray);
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
int* findvalid(Data* data){
    int info1, info2, length = 0;
    int* result = (int*)malloc(2 * directionsize * sizeof(int));
    printf("BEFORE INFO!-1\n");
    if (data->data1[3] > -1 && data->data1[4] > -1){
        info1 = newnode[data->data1[3]][data->data1[4]]->frame;
    }else{
        info1 = -1;
    }
    info2 = newnode[data->data1[0]][data->data1[1]]->frame;
    for (int k = 0;k < directionsize;k++){
        if ((data->data1[0] + directions[k][0] != data->data1[3] ||
            data->data1[1] + directions[k][1] != data->data1[4]) &&
            data->data1[0] + directions[k][0] < ROWS &&
            data->data1[0] + directions[k][0] > -1 &&
            data->data1[1] + directions[k][1] < COLUMNS &&
            data->data1[1] + directions[k][1] > -1 &&
            data->board[data->data1[0] + directions[k][0]][data->data1[1] + directions[k][1]] == 0 &&
            newnode[data->data1[0] + directions[k][0]][data->data1[1] + directions[k][1]]->frame != info1 &&
            newnode[data->data1[0] + directions[k][0]][data->data1[1] + directions[k][1]]->frame != info2){
                result[2 * length] = data->data1[0] + directions[k][0];
                result[2 * length + 1] = data->data1[1] + directions[k][1];
                length++;
        }
    }
    validlength = length-1;
    for(int k = length;k < directionsize;k++){
        result[2 * k] = -1;
        result[2 * k + 1] = -1;
    }
    return result;
}

bool check_done_my(int** board,int x, int y){
    // Use memory pool for better performance
    int** copy = get_board_from_pool();
    for(int i = 0; i < ROWS; i++) {
        memcpy(copy[i], board[i], COLUMNS * sizeof(int));
    }
    copy[x][y] = 1;

    // Initialize socket connection if not already done
    if (!socket_initialized) {
        cached_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (cached_socket < 0) {
            perror("Socket creation failed");
            return_board_to_pool(copy);
            return 0;
        }

        // Set socket options to allow reuse of address
        int opt = 1;
        if (setsockopt(cached_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt failed");
            close(cached_socket);
            cached_socket = -1;
            return_board_to_pool(copy);
            return 0;
        }

        // Set up server address
        memset(&cached_server_addr, 0, sizeof(cached_server_addr));
        cached_server_addr.sin_family = AF_INET;
        cached_server_addr.sin_port = htons(PORT);
        cached_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        socket_initialized = true;
    }

    // Try to connect (reuse existing socket if possible)
    if (connect(cached_socket, (struct sockaddr*)&cached_server_addr, sizeof(cached_server_addr)) < 0) {
        perror("Connection failed");
        close(cached_socket);
        cached_socket = -1;
        socket_initialized = false;
        return_board_to_pool(copy);
        return 0;
    }

    printf("Connected to server\n");

    // Create a flat array to send data properly
    int flat_array[ROWS * COLUMNS];
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            flat_array[i * COLUMNS + j] = copy[i][j];
        }
    }

    // Send data
    if (send(cached_socket, flat_array, ROWS * COLUMNS * sizeof(int), 0) < 0) {
        perror("Send failed");
        close(cached_socket);
        cached_socket = -1;
        socket_initialized = false;
        return_board_to_pool(copy);
        return 0;
    }
    printf("Board sent to server\n");

    // Receive result
    bool result;
    if (recv(cached_socket, &result, sizeof(result), 0) < 0) {
        perror("Receive failed");
        close(cached_socket);
        cached_socket = -1;
        socket_initialized = false;
        return_board_to_pool(copy);
        return 0;
    }
    printf("Server response: %d\n", result);

    // Close connection and clean up
    close(cached_socket);
    cached_socket = -1;
    socket_initialized = false;
    return_board_to_pool(copy);

    return result ? 1 : 0;
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
        *result = calculate(2,data->board);
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
    data->board[data->data1[0]][data->data1[1]] = data->data1[5]; // THIS MAY BE UNNECESSARY

    if (data->data1[5] == 2){
        data->data1[5] = 1;
    }else if (data->data1[5] == 1){
        data->data1[5] = 2;
    }else{
        printf("COLOR ERROR %d",data->data1[5]);
    }
    if(data->ret){
        printf("START SEARCH\n)");
    }

    bool piece_check = piece_count(data->board) < 56;
    bool got_into = false;

    for(int k = 0;k < directionsize;k++){
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
    if(!got_into){
        maximum = calculate(2,data->board);
    }

    free(datas);
    if (data->ret){
        return result;
    }

    *result = maximum;
    return result;
}

int* best_place(int x, int y,int step, int lx, int ly){
    int i, j;

    board2[x][y] = 1;

    genstep = step;
    i = newnode[x][y]->frame;
    if (i != userframe && i != pcframe){
        userframe = i;
    }
    else{
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
    data.data1[5] = 2;
    data.data1[6] = -999999999;
    data.data1[7] = 999999999;
    data.is_max = true;
    data.ret = true;
    int** board = get_board_from_pool();
    for (i = 0;i < ROWS;i++){
        memcpy(board[i],board2[i], COLUMNS * sizeof(int));
    }

    data.board = board;


    file = fopen("data.txt","a");
    for(int i = 0;i < ROWS;i++){
        for(int j = 0;j < COLUMNS;j++){
            fprintf(file,"%d ",board2[i][j]);
        }
    }
    fprintf(file, "---> ");

    temp = search((void*)&data);
    fprintf(file,"(%d,%d)\n",temp[0],temp[1]);
    fclose(file);

    // Clean up memory
    return_board_to_pool(board);
    free(data.data1);

    printf("X: %d Y: %d\n",temp[0],temp[1]);
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
    // Initialize memory pool for better performance
    init_board_pool();
    
    board2 = (int**)malloc(ROWS * sizeof(int*));
    for(int i = 0;i < ROWS;i++){
        board2[i] = (int*)malloc(COLUMNS * sizeof(int));
    }
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
