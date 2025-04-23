#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define THREADSIZE 13
#define calcfuncsize 6
#define rows 8
#define columns 8
#define directionsize 28

// MARBLE KESIN SORUNLU hepsi 0 gözüküyo
// PLACE yanlış hesaplıyo
// 135 de yanlış hesaplıyo
// 45 de yanlış hesaplıyo (135 ile aynı sonuç çıkıyo)
// vertical da horizontal da yanlış

int area = 0, userframe = -1, pcframe = -1;
int genstep = -1;

int board2[8][8];
FILE* file ;

int marble_result,oneslen = 0;

int** ones;

bool** checked;

int directions[directionsize][2] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7},
    {0, -1}, {0, -2}, {0, -3}, {0, -4}, {0, -5}, {0, -6}, {0, -7},
    {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0},
    {-1, 0}, {-2, 0}, {-3, 0}, {-4, 0}, {-5, 0}, {-6, 0}, {-7, 0}
};

int frames[17][13] = {{4,0,0,0,1,1,0,1,1,-1,-1,-1,-1},
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
    void* (*func)(void*);
    int result;
    bool returned;
}Data2;

typedef struct Node {
    int frame;
}Node;
Node* newnode[8][8];
typedef struct {
    int x;
    int y;
    int step;
    int not_x;
    int not_y;
    int color;
    bool ret;
    int** board;
    int* path;
    int result;
    bool returned;
}Data;

void print_board(int** board){
    int i, j;
    for(i = 0;i < 8;i++){
        for(j = 0;j < 8;j++){
            printf("%d ",board[i][j]);
        }
        printf("\n");
    }
}
void* horizontal_points(void *arg){
    int i,j,length, pc = 0,user = 0;

    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        printf("HEREEEEEEEEEEEEEEEEEEE HORIZONTAL\n");
        perror("Failed to allocate memory horizontal");
        pthread_exit(NULL);
    }
    Data2* data = (Data2*)arg;
    /*if (!data || !data->board || !data->board[0]) {
        printf("Invalid board dataaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
    }*/

    int** board = (int**)malloc(8 * sizeof(int*));
    for (i = 0;i < 8;i++){
        board[i] = (int*)malloc(8 * sizeof(int));
        memcpy(board[i],data->board[i],8 * sizeof(int));
    }

    int color = data->color;

    for (i = 0;i < 8;i++){
        length = 1;
        for (j = 1;j < 8;j++){

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
    *result = pc - user;

    print_board(board);
    for (i = 0;i < 8;i++){
        free(board[i]);
    }

    free(board);

    printf("HORIZONTAL RETURNS %d\n", pc-user);
    return (void*)result;
}
void* vertical_points(void *arg){
    int i,j,length,pc = 0,user = 0;

    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(8 * sizeof(int*));
    for (i = 0;i < 8;i++){
        board[i] = (int*)malloc(8 * sizeof(int));
        memcpy(board[i],data->board[i],8 * sizeof(int));
    }

    int color = data->color;

    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        printf("HEREEEEEEEEEEEEEEEEEEE VERTICAL\n");
        perror("Failed to allocate memory vertical");
        pthread_exit(NULL);
    }

    for (j = 0;j < 8;j++){
        length = 1;
        for (i = 1;i < 8;i++){

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

    *result = pc - user;
    print_board(board);
    for (i = 0;i < 8;i++){
        free(board[i]);
    }

    free(board);

    printf("VERTICAL RETURNS %d\n", pc-user);
    return (void*)result;
}
void* diagonal_points_45(void *arg){
    int i , length, pc = 0, user = 0,x,y;
    int places[7][2] = {
        {7,0},{6,0},{5,0},{4,0},{7,3},{7,2},{7,1}
    };

    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(8 * sizeof(int*));
    for (i = 0;i < 8;i++){
        board[i] = (int*)malloc(8 * sizeof(int));
        memcpy(board[i],data->board[i],8 * sizeof(int));
    }
    int color = data->color;

    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        printf("HEREEEEEEEEEEEEEEEEEEE 45\n");
        perror("Failed to allocate memory 45");
        pthread_exit(NULL);
    }


    for(i = 0;i < 7;i++){
        x = places[i][0];
        y = places[i][1];
        length = 1;
        while (x < 8 && y < 8 && x > -1 && y > -1){
            if (x+1 < 8 && x+1 > -1 && y-1 < 8 && y-1 > -1 && board[x][y] != 0 && board[x][y] == board[x+1][y-1]){
                length++;
            }
            else{
                if (length >= 5){
                    if (x+1 < 8 && x+1 > -1 && y-1 < 8 && y-1 > -1 && board[x+1][y-1] == 2){
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
    print_board(board);
    for (i = 0;i < 8;i++){
        free(board[i]);
    }
    free(board);
    printf("45 RETURNS %d\n", pc-user);
    return (void*)result;
}
void* diagonal_points_135(void *arg){
    int i, length, pc = 0, user = 0,x,y;
    int places[7][2] = {
        {7,4},{6,7},{5,7},{4,7},{7,5},{7,6},{7,7}
    };

    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(8 * sizeof(int*));
    for (i = 0;i < 8;i++){
        board[i] = (int*)malloc(8 * sizeof(int));
        memcpy(board[i],data->board[i],8 * sizeof(int));
    }
    int color = data->color;

    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        printf("HEREEEEEEEEEEEEEEEEEEE 135\n");
        perror("Failed to allocate memory 135");
        pthread_exit(NULL);
    }

    for(i = 0;i < 7;i++){
        x = places[i][0];
        y = places[i][1];
        length = 0;
        while (x > -1 && y > -1){
            if (x+1 < 8 && x+1 > -1 && y+1 < 8 && y+1 > -1 && board[x][y] != 0 && board[x][y] == board[x+1][y+1]){
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
    print_board(board);
    for (i = 0;i < 8;i++){
        free(board[i]);
    }
    free(board);
    printf("135 RETURNS %d\n", pc-user);
    return (void*)result;
}
int dfs(int i,int j,int** board,int color){
    int k,m,n,temp = 0;

    if (i < 0 || i == rows || j < 0 || j == columns || board[i][j] != color) {
        return 0;
    }
    board[i][j] = 0;
    return 1 + dfs(i+1,j,board,color) + dfs(i-1,j,board,color) + dfs(i,j+1,board,color) + dfs(i,j-1,board,color);
}
void* marble_area_points(void *arg){
    Data2* data = (Data2*)arg;
    int i, j,user = 0, pc = 0, temp;
    int** board = (int**)malloc(8 * sizeof(int*));
    for (i = 0;i < 8;i++){
        board[i] = (int*)malloc(8 * sizeof(int));
        memcpy(board[i],data->board[i],8 * sizeof(int));
    }

    int color = data->color;
    print_board(board);
    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        printf("HEREEEEEEEEEEEEEEEEEEE MARBLE\n");
        perror("Failed to allocate memory marble area");
        pthread_exit(NULL);
    }

    for(i = 0;i < rows;i++){
        for(j = 0;j < columns;j++){
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

    for (i = 0;i < 8;i++){
        free(board[i]);
    }
    free(board);
    printf("MARBLE RETURNS %d\n", pc-user);
    return (void*)result;
}
void* place_area_points(void *arg){
    int i,j,pc = 0,user = 0,length = 0;
    int* result = (int*)malloc(sizeof(int));
    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(8 * sizeof(int*));
    for (i = 0;i < 8;i++){
        board[i] = (int*)malloc(8 * sizeof(int));
        memcpy(board[i],data->board[i],8 * sizeof(int));
    }
    int color = data->color;

    if (result == NULL) {
        printf("HEREEEEEEEEEEEEEEEEEEE PLACE\n");
        perror("Failed to allocate memory place area");
        pthread_exit(NULL);
    }

    for (i = 0;i < 17;i++){
        pc = 0;
        user = 0;
        for (j = 1; j < frames[i][0]; j += 2){
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
    print_board(board);
    for (i = 0;i < 8;i++){
        free(board[i]);
    }

    free(board);
    printf("PLACE RETURNS %d\n", length);
    return (void*)result;
}
Data2 copydata2(Data2* data){
    Data2 copy = *data;

    copy.board = (int**)malloc(8 * sizeof(int*));
    for (int i = 0; i < 8; i++) {
        copy.board[i] = (int*)malloc(8 * sizeof(int));
        memcpy(copy.board[i], data->board[i], 8 * sizeof(int));
    }
    return copy;
}
void freedata2(Data2 data){
    for (int i = 0; i < 8; i++) {
        free(data.board[i]);
    }
    free(data.board);
}

int* calculate(int color, int** board){

    pthread_t* calcthreads = (pthread_t*)malloc(calcfuncsize * sizeof(pthread_t));
    int i;
    int** result = (int**)malloc(calcfuncsize * sizeof(int*));
    for(i = 0;i < calcfuncsize;i++){
        result[i] = (int*)malloc(sizeof(int));
    }
    int resultindex = 0;
    Data2* data = (Data2*)malloc(sizeof(Data2));
    data->color = color;
    data->board = (int**)malloc(8 * sizeof(int*));
    for (int i = 0;i < 8;i++){
        data->board[i] = (int*)malloc(8 * sizeof(int));
        memcpy(data->board[i],board[i],8 * sizeof(int));
    }
    int ab;
    Data2* parray = (Data2*)malloc(calcfuncsize * sizeof(Data2));
    for(i = 0;i < calcfuncsize;i++){
        parray[i] = copydata2(data);
    }

    parray[0].func = horizontal_points;
    parray[1].func = vertical_points;
    parray[2].func = diagonal_points_45;
    parray[3].func = diagonal_points_135;
    parray[4].func = place_area_points;
    parray[5].func = marble_area_points;
    for(i = 0;i < calcfuncsize;i++){
        parray[i].result = -1;
        parray[i].returned = false;
    }
    int* sum = (int*)malloc(1 * sizeof(int));
    *sum =  *(int*)horizontal_points((void*)&parray[0])  +
            *(int*)vertical_points((void*)&parray[1])    +
            *(int*)diagonal_points_45((void*)&parray[2]) +
            *(int*)diagonal_points_135((void*)&parray[3])+
            *(int*)place_area_points((void*)&parray[4])  +
            *(int*)marble_area_points((void*)&parray[5]);

    //
    /*for(i = 0;i < calcfuncsize;i++){
        free(result[i]);
    }
    free(result);
    free(calcthreads);
    free(data);
    for(i = 0;i < calcfuncsize;i++){
        freedata2(parray[i]);
    }
    free(parray);*/
    return sum;
}

int which(int x, int y){
    int j;
    int i;

    for (i = 0;i < 17;i++){
        for (j = 1;j < 2 * frames[i][0];j += 2){
            if (frames[i][j] == x && frames[i][j+1] == y){
                return i;
            }
        }
    }
}
void append(Data *data){

    int i;
    for(i = 0;i < 33;i++){
        fprintf(file,"%d,",data->path[i]);
    }
    fprintf(file, "\n");
}
void* search(void *arg){
    int i,j,k,length = 0,maximum = -1,info1,info2,a,b;
    Data* data = (Data*)arg;
    int x = data->x;
    int y = data->y;
    int step = data->step;
    int not_x = data->not_x;
    int not_y = data->not_y;
    int color = data->color;
    bool ret = data->ret;
    int** board = (int**)malloc(8 * sizeof(int*));
    int* result2 = (int*)malloc(33 * sizeof(int));
    for(i = 0;i < 8;i++){
        board[i] = (int*)malloc(8 * sizeof(int));
        memcpy(board[i],data->board[i],8 * sizeof(int));
    }
    memcpy(result2, data->path, 33 * sizeof(int));
    Data** datas = (Data**)malloc(directionsize * sizeof(Data*));
    // normalde her birine malloc yapmaya gerek yok
    //ama freelerken double free olmaması için yapıyom şuanlık ilerde geliştirilebilir
    for (i = 0;i < directionsize;i++){
        datas[i] = (Data*)malloc(sizeof(Data));
    }
    int** array;
    int* result;
    int* invalid;
    invalid = (int*)malloc(1 * sizeof(int));
    result = (int*)malloc(2 * sizeof(int));
    array = (int**)malloc(1 * sizeof(int*));
    if (not_x > -1 && not_y > -1){
        info1 = newnode[not_x][not_y]->frame;
    }else{
        info1 = -1;
    }
    info2 = newnode[x][y]->frame;
    board[x][y] = color;
    if (step == 0){
        *invalid = *calculate(2,board);
        free(result);
        data->path[0] = *invalid;
        append(data);
        return (void*)invalid;
    }

    if (color == 2){
        color = 1;
    }else if (color == 1){
        color = 2;
    }else{
        printf("COLOR ERROR %d",color);
    }
    int* used = (int*)malloc((THREADSIZE-calcfuncsize) * sizeof(int));
    int usedindex = 0;
    for (k = 0;k < directionsize;k++){
        if ((x + directions[k][0] != not_x ||
            y + directions[k][1] != not_y) &&
            x + directions[k][0] < 8 &&
            x + directions[k][0] > -1 &&
            y + directions[k][1] < 8 &&
            y + directions[k][1] > -1 &&
            board[x + directions[k][0]][y + directions[k][1]] == 0 &&
            newnode[x + directions[k][0]][y + directions[k][1]]->frame != info1 &&
            newnode[x + directions[k][0]][y + directions[k][1]]->frame != info2){

            length++;
            used[usedindex] = k;
            usedindex++;
            //datas[k] = (Data*)malloc(sizeof(Data));
            board[x + directions[k][0]][y + directions[k][1]] = color;
            array = (int**)realloc(array,length * sizeof(int*));
            array[length-1] = (int*)malloc(3 * sizeof(int));
            datas[k]->x = x + directions[k][0];
            datas[k]->y = y + directions[k][1];
            datas[k]->step = step - 1;
            datas[k]->not_x = x;
            datas[k]->not_y = y;
            datas[k]->color = color;
            datas[k]->ret = false;
            datas[k]->board = (int**)malloc(8 * sizeof(int*));
            for (i = 0; i < 8; i++) {
                datas[k]->board[i] = (int*)malloc(8 * sizeof(int));
                memcpy(datas[k]->board[i], board[i], 8 * sizeof(int));
            }

            result2[(2*genstep)-(2*step)+3] = x + directions[k][0];
            result2[(2*genstep)-(2*step)+4] = y + directions[k][1];
            datas[k]->path = (int*)malloc(33 * sizeof(int));
            memcpy(datas[k]->path, result2, 33 * sizeof(int));
            datas[k]->returned = false;
            datas[k]->result = -1;

            // NORMAL ÇAĞIR İŞTE YAV
            array[length-1][0] = *(int*)search((void*)datas[k]);
            array[length-1][1] = x + directions[k][0];
            array[length-1][2] = y + directions[k][1];
            board[x + directions[k][0]][y + directions[k][1]] = 0;
        }
    }
    int** results = (int**)malloc(directionsize * sizeof(int*));

    for(i = 0;i < length;i++){
        if (array[i][0] > maximum){
            maximum = array[i][0];
            result[0] = array[i][1];
            result[1] = array[i][2];
            //free(array[i]);
        }
    }
    //free(array);

    if (ret){
        return (void*)result;
    }

    *invalid = maximum;
    return (void*)invalid;
}

int* best_place(int x, int y,int step, int lx, int ly){
    int x_start,y_start;
    int i,j;
    x_start = x;
    y_start = y;

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

    file = fopen("data.txt","a");
    int* temp;
    Data data;
    data.x = x;
    data.y = y;
    data.step = step;
    data.not_x = lx;
    data.not_y = ly;
    data.color = 2;
    data.ret = true;
    data.result = -1;
    data.returned = false;

    int** board3 = (int**)malloc(8 * sizeof(int*));
    for (i = 0;i < 8;i++){
        board3[i] = (int*)malloc(8 * sizeof(int));
    }
    for (i = 0;i < 8;i++){
        for (j = 0;j < 8;j++){
            board3[i][j] = board2[i][j];
        }
    }
    data.board = board3;
    data.path = (int*)malloc(33 * sizeof(int));
    for (i = 3;i < 33;i++){
        data.path[i] = -1;
    }
    data.path[0] = -1;
    data.path[1] = x;
    data.path[2] = y;

    temp = (int*)search((void*)&data);
    board2[temp[0]][temp[1]] = 2;

    j = newnode[temp[0]][temp[1]]->frame;
    if (j != userframe && j != pcframe){
        pcframe = j;
    }
    else{
        printf("PC FRAME ERROR\n");
        printf("PC FRAME = %d\n",pcframe);
        printf("USER FRAME = %d\n",userframe);
        printf("j = %d\n",j);
    }

    fclose(file);
    return temp;
}

int main(){
    int i,j;
    ones = (int**)malloc(1 * sizeof(int*));
    // make everywhere empty
    for (i = 0;i < 8; i++){
        for (j = 0;j < 8;j++){
            board2[i][j] = 0;
        }
    }
    // make checked array empty
    checked = (bool**)malloc(8 * sizeof(bool*));
    for(i = 0;i < 8;i++){
        checked[i] = (bool*)malloc(8 * sizeof(bool));
        for (j = 0;j < 8;j++){
            checked[i][j] = false;
        }
    }

    for (i = 0;i < 8;i++){
        for(j = 0;j < 8;j++){
            newnode[i][j] = (Node*)malloc(sizeof(Node));
            newnode[i][j]->frame = which(i,j);
        }
    }
    return 0;
}
