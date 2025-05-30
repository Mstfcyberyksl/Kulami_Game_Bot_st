#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define calcfuncsize 6
#define ROWS 8
#define COLUMNS 8
#define directionsize 28
#define framecount 17
#define data_length 8
#define PORT 9000

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


int directions[directionsize][2] = {
    {0, -1}, {0, -2}, {0, -3}, {0, -4}, {0, -5}, {0, -6}, {0, -7},
    {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7},
    {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0},
    {-1, 0}, {-2, 0}, {-3, 0}, {-4, 0}, {-5, 0}, {-6, 0}, {-7, 0}
};

int frames[framecount][13] = {{4,0,0,0,1,1,0,1,1,-1,-1,-1,-1},
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

void freedata2(Data2* data){
    for (int i = 0; i < ROWS; i++) {
        free(data->board[i]);
    }
    free(data->board);
    free(data->horizontal_info);
    free(data->vertical_info);
}

void freedata(Data* data){
    for(int i = 0; i < ROWS; i++){
        free(data->board[i]);
    }
    free(data->board);
    free(data->data1);
}

void print_board(int** board){
    for(int i = 0;i < ROWS;i++){
        for(int j = 0;j < COLUMNS;j++){
            printf("%d ",board[i][j]);
        }
        printf("\n");
    }
}
void* horizontal_points(void *arg){
    int length, pc = 0,user = 0;

    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        perror("Failed to allocate memory in horizontal_points function");
        pthread_exit(NULL);
    }
    Data2* data = (Data2*)arg;

    int** board = (int**)malloc(ROWS * sizeof(int*));
    for (int i = 0;i < ROWS;i++){
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(board[i],data->board[i],COLUMNS * sizeof(int));
    }
    int color = data->color;
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
    *result = pc - user;
    for (int i = 0;i < ROWS;i++){
        free(board[i]);

    }
    free(board);
    return (void*)result;
}
void* vertical_points(void *arg){
    int i,j,length,pc = 0,user = 0;

    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(ROWS * sizeof(int*));
    for (i = 0;i < ROWS;i++){
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(board[i],data->board[i],COLUMNS * sizeof(int));
    }

    int color = data->color;

    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        perror("Failed to allocate memory in vertical_points function");
        pthread_exit(NULL);
    }
    for (j = 0;j < ROWS;j++){
        if(data->vertical_info[j] >= 5){
            length = 1;
            for (i = 1;i < COLUMNS;i++){
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

    *result = pc - user;
    for (i = 0;i < ROWS;i++){
        free(board[i]);
    }
    free(board);
    return (void*)result;
}
void* diagonal_points_45(void *arg){
    int length, pc = 0, user = 0, x, y;
    int places[7][2] = {
        {7,0},{6,0},{5,0},{4,0},{7,3},{7,2},{7,1}
    };
    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(ROWS * sizeof(int*));
    for (int i = 0;i < ROWS;i++){
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(board[i],data->board[i],COLUMNS * sizeof(int));
    }
    int color = data->color;

    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        printf("HEREEEEEEEEEEEEEEEEEEE 45\n");
        perror("Failed to allocate memory 45");
        pthread_exit(NULL);
    }

    for(int i = 0;i < 7;i++){
        x = places[i][0];
        y = places[i][1];
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

    *result = pc - user;
    for (int i = 0;i < ROWS;i++){
        free(board[i]);
    }
    free(board);
    //free(data); // HERE
    return (void*)result;
}
void* diagonal_points_135(void *arg){
    int length, pc = 0, user = 0,x,y;
    int places[7][2] = {
        {7,4},{6,7},{5,7},{4,7},{7,5},{7,6},{7,7}
    };

    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(ROWS * sizeof(int*));
    for (int i = 0;i < ROWS;i++){
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(board[i],data->board[i],COLUMNS * sizeof(int));
    }
    int color = data->color;

    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        printf("HEREEEEEEEEEEEEEEEEEEE 135\n");
        perror("Failed to allocate memory 135");
        pthread_exit(NULL);
    }

    for(int i = 0;i < 7;i++){
        x = places[i][0];
        y = places[i][1];
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

    *result = pc - user;
    for (int i = 0;i < ROWS;i++){
        free(board[i]);
    }
    free(board);
    //free(data); // HERE
    return (void*)result;
}
int dfs(int i,int j,int** board,int color){
    int k,m,n,temp = 0;

    if (i < 0 || i == ROWS || j < 0 || j == COLUMNS || board[i][j] != color) {
        return 0;
    }
    board[i][j] = 0;
    return 1 + dfs(i+1,j,board,color) + dfs(i-1,j,board,color) + dfs(i,j+1,board,color) + dfs(i,j-1,board,color);
}
void* marble_area_points(void *arg){
    Data2* data = (Data2*)arg;
    int user = 0, pc = 0, temp;
    int** board = (int**)malloc(ROWS * sizeof(int*));
    for (int i = 0;i < ROWS;i++){
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(board[i],data->board[i],COLUMNS * sizeof(int));
    }
    int color = data->color;

    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        perror("Failed to allocate memory marble area");
        pthread_exit(NULL);
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

    *result = pc - user;

    for (int i = 0;i < ROWS;i++){
        free(board[i]);
    }
    free(board);
    //free(data); // HERE

    return (void*)result;
}
void* place_area_points(void *arg){
    int pc = 0,user = 0,length = 0;
    int* result = (int*)malloc(sizeof(int));
    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(ROWS * sizeof(int*));
    for (int i = 0;i < ROWS;i++){
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(board[i],data->board[i],COLUMNS * sizeof(int));
    }
    int color = data->color;

    if (result == NULL) {
        printf("HEREEEEEEEEEEEEEEEEEEE PLACE\n");
        perror("Failed to allocate memory place area");
        pthread_exit(NULL);
    }

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
    *result = length;
    for (int i = 0;i < ROWS;i++){
        free(board[i]);
    }
    free(board);
    //free(data); // HERE
    return (void*)result;
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


int* calculate(int color, int** board){
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
    int** temp;
    temp = (int**)malloc(calcfuncsize * sizeof(int*));

    temp[0] = horizontal_points((void*)parray[0]);
    temp[1] = vertical_points((void*)parray[1]);
    temp[2] = diagonal_points_45((void*)parray[2]);
    temp[3] = diagonal_points_135((void*)parray[3]);
    temp[4] = place_area_points((void*)parray[4]);
    temp[5] = marble_area_points((void*)parray[5]);
    int* sum = (int*)malloc(sizeof(int));
    *sum = *(int*)temp[0] + *(int*)temp[1] + *(int*)temp[2] + *(int*)temp[3] + *(int*)temp[4] + *(int*)temp[5];

    for(int i = 0;i < calcfuncsize; i++){
        freedata2(parray[i]);
        free(parray[i]);
        free(temp[i]);
    }
    free(parray);
    free(temp);
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
void append(Data *data){
    /*file = fopen("data.txt","a");
    for(int i = 0;i < path_size;i++){
        fprintf(file,"%d,",data->path[i]);
    }
    fprintf(file, "\n");
    fclose(file);*/
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
    printf("FINITO\n");
    for(int k = length;k < directionsize;k++){
        result[2 * k] = -1;
        result[2 * k + 1] = -1;
    }
    return result;
}

bool check_done_my(int** board,int x, int y){
    int** copy = (int**)malloc(ROWS * sizeof(int*));
    for(int i = 0; i < ROWS; i++) {
        copy[i] = (int*)malloc(COLUMNS * sizeof(int));
        memcpy(copy[i], board[i], COLUMNS * sizeof(int));
    }
    copy[x][y] = 1;

    // Create socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        // Clean up memory before returning
        for(int i = 0; i < ROWS; i++) {
            free(copy[i]);
        }
        free(copy);
        return 0; // Return 0 for failure
    }

    // Set socket options to allow reuse of address
    int opt = 1;
    if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(client_fd);
        // Clean up memory
        for(int i = 0; i < ROWS; i++) {
            free(copy[i]);
        }
        free(copy);
        return 0;
    }

    // Set up server address
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Use localhost for testing or get the correct IP
    // For production, replace with actual server IP
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Use localhost for testing

    // Try to connect with timeout
    printf("Attempting to connect to server...\n");
    if (connect(client_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        close(client_fd);
        // Clean up memory
        for(int i = 0; i < ROWS; i++) {
            free(copy[i]);
        }
        free(copy);
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
    if (send(client_fd, flat_array, ROWS * COLUMNS * sizeof(int), 0) < 0) {
        perror("Send failed");
        close(client_fd);
        // Clean up memory
        for(int i = 0; i < ROWS; i++) {
            free(copy[i]);
        }
        free(copy);
        return 0;
    }
    printf("Board sent to server\n");

    // Receive result
    bool result;
    if (recv(client_fd, &result, sizeof(result), 0) < 0) {
        perror("Receive failed");
        close(client_fd);
        // Clean up memory
        for(int i = 0; i < ROWS; i++) {
            free(copy[i]);
        }
        free(copy);
        return 0;
    }
    printf("Server response: %d\n", result);

    // Close connection and clean up
    close(client_fd);
    for(int i = 0; i < ROWS; i++) {
        free(copy[i]);
    }
    free(copy);

    return result ? 1 : 0;
}

int piece_count(int** board){
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
void* search(void *arg){
    Data* data = (Data*)arg;
    if (data->data1[2] == 0){
        int* result = (int*)calculate(2,data->board);
        return (void*)result;
    }

    int  info1, info2, a, b;
    int* maximum = (int*)malloc(sizeof(int));
    if (data->is_max){
        *maximum = -1000000;
    }else{
        *maximum = 1000000;
    }
    Data** datas = (Data**)malloc(directionsize * sizeof(Data*));

    int* result;

    result = (int*)malloc(2 * sizeof(int));
    result[0] = -1;
    result[1] = -1;

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
    if(data->ret){
        printf("START SEARCH\n)");
    }
    bool piece_check = piece_count(data->board) < 56;
    bool got_into = false;

    for (int k = 0;k < directionsize;k++){
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
            got_into = true;
            data->board[data->data1[0] + directions[k][0]][data->data1[1] + directions[k][1]] = data->data1[5];

            datas[k] = (Data*)malloc(sizeof(Data));
            datas[k]->data1 = (int*)malloc(data_length * sizeof(int));
            datas[k]->data1[0] = data->data1[0] + directions[k][0];
            datas[k]->data1[1] = data->data1[1] + directions[k][1];
            datas[k]->data1[2] = data->data1[2] - 1;
            datas[k]->data1[3] = data->data1[0];
            datas[k]->data1[4] = data->data1[1];
            datas[k]->data1[5] = data->data1[5];
            datas[k]->data1[6] = data->data1[6];
            datas[k]->data1[7] = data->data1[7];
            datas[k]->is_max = !data->is_max;
            datas[k]->ret = false;
            datas[k]->board = (int**)malloc(ROWS * sizeof(int*));
            for (int i = 0; i < ROWS; i++) {
                datas[k]->board[i] = (int*)malloc(COLUMNS * sizeof(int));
                memcpy(datas[k]->board[i], data->board[i], COLUMNS * sizeof(int));
            }


            int* temppp;
            temppp = search((void*)datas[k]);
            if (data->is_max){
                if (*temppp > *maximum){
                    *maximum = *temppp;
                    result[0] = data->data1[0] + directions[k][0];
                    result[1] = data->data1[1] + directions[k][1];
                }
                if(*maximum > data->data1[6] ){
                    data->data1[6] = *maximum;
                }

                if (data->data1[6] >= data->data1[7]){
                    free(temppp);
                    freedata(datas[k]);
                    free(datas[k]);
                    data->board[data->data1[0] + directions[k][0]][data->data1[1] + directions[k][1]] = 0;

                    break;
                }
            }else{

                if (*temppp < *maximum){
                    *maximum = *temppp;
                    result[0] = data->data1[0] + directions[k][0];
                    result[1] = data->data1[1] + directions[k][1];
                }


                if(*maximum < data->data1[7] ){
                    data->data1[7] = *maximum;
                }

                if (data->data1[7] <= data->data1[6]){
                    free(temppp);
                    freedata(datas[k]);
                    free(datas[k]);
                    data->board[data->data1[0] + directions[k][0]][data->data1[1] + directions[k][1]] = 0;

                    break;
                }

            }

            free(temppp);
            freedata(datas[k]);
            free(datas[k]);
            data->board[data->data1[0] + directions[k][0]][data->data1[1] + directions[k][1]] = 0;
        }
    }
    if(!got_into){
        *maximum = *(int*)calculate(2,data->board);
    }
    free(datas);

    if (data->ret){
        return (void*)result;
    }
    free(result);

    return (void*)maximum;
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
    int** board = (int**)malloc(ROWS * sizeof(int*));
    for (i = 0;i < ROWS;i++){
        board[i] = (int*)malloc(COLUMNS * sizeof(int));
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

    temp = (int*)search((void*)&data);
    fprintf(file,"(%d,%d)\n",temp[0],temp[1]);
    fclose(file);

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
