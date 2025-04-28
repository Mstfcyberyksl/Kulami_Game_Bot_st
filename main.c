#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define calcfuncsize 6
#define rows 8
#define columns 8
#define directionsize 28
#define framecount 17
#define path_size 33

int area = 0, userframe = -1, pcframe = -1;
int genstep = -1;

int** board2;
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
}Data2;

typedef struct Node {
    int frame;
}Node;
Node* newnode[rows][columns];
typedef struct {
    int x;
    int y;
    int step;
    int not_x;
    int not_y;
    int color;
    bool ret;
    int** board;
}Data;

void freedata2(Data2* data){
    for (int i = 0; i < rows; i++) {
        free(data->board[i]);
    }
    free(data->board);
}

void freedata(Data* data){
    for(int i = 0; i < rows; i++){
        free(data->board[i]);
    }
    free(data->board);
}

void print_board(int** board){
    for(int i = 0;i < rows;i++){
        for(int j = 0;j < columns;j++){
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

    int** board = (int**)malloc(rows * sizeof(int*));
    for (int i = 0;i < rows;i++){
        board[i] = (int*)malloc(columns * sizeof(int));
        memcpy(board[i],data->board[i],columns * sizeof(int));
    }
    int color = data->color;

    for (int i = 0;i < rows;i++){
        length = 1;
        for (int j = 1;j < columns;j++){
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
    for (int i = 0;i < rows;i++){
        free(board[i]);

    }
    free(board);
    //free(data); // HERE
    return (void*)result;
}
void* vertical_points(void *arg){
    int i,j,length,pc = 0,user = 0;

    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(rows * sizeof(int*));
    for (i = 0;i < rows;i++){
        board[i] = (int*)malloc(columns * sizeof(int));
        memcpy(board[i],data->board[i],columns * sizeof(int));
    }

    int color = data->color;

    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        perror("Failed to allocate memory in vertical_points function");
        pthread_exit(NULL);
    }

    for (j = 0;j < rows;j++){
        length = 1;
        for (i = 1;i < columns;i++){
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
    for (i = 0;i < rows;i++){
        free(board[i]);
    }
    free(board);
    //free(data); // HERE
    return (void*)result;
}
void* diagonal_points_45(void *arg){
    int length, pc = 0, user = 0, x, y;
    int places[7][2] = {
        {7,0},{6,0},{5,0},{4,0},{7,3},{7,2},{7,1}
    };
    Data2* data = (Data2*)arg;
    int** board = (int**)malloc(rows * sizeof(int*));
    for (int i = 0;i < rows;i++){
        board[i] = (int*)malloc(columns * sizeof(int));
        memcpy(board[i],data->board[i],columns * sizeof(int));
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
        while (x < rows && y < columns && x > -1 && y > -1){
            if (x+1 < rows && x+1 > -1 && y-1 < columns && y-1 > -1 && board[x][y] != 0 && board[x][y] == board[x+1][y-1]){
                length++;
            }
            else{
                if (length >= 5){
                    if (x+1 < rows && x+1 > -1 && y-1 < columns && y-1 > -1 && board[x+1][y-1] == 2){
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
    for (int i = 0;i < rows;i++){
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
    int** board = (int**)malloc(rows * sizeof(int*));
    for (int i = 0;i < rows;i++){
        board[i] = (int*)malloc(columns * sizeof(int));
        memcpy(board[i],data->board[i],columns * sizeof(int));
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
            if (x+1 < rows && x+1 > -1 && y+1 < columns && y+1 > -1 && board[x][y] != 0 && board[x][y] == board[x+1][y+1]){
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
    for (int i = 0;i < rows;i++){
        free(board[i]);
    }
    free(board);
    //free(data); // HERE
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
    int user = 0, pc = 0, temp;
    int** board = (int**)malloc(rows * sizeof(int*));
    for (int i = 0;i < rows;i++){
        board[i] = (int*)malloc(columns * sizeof(int));
        memcpy(board[i],data->board[i],columns * sizeof(int));
    }
    int color = data->color;

    int* result = (int*)malloc(sizeof(int));
    if (result == NULL) {
        perror("Failed to allocate memory marble area");
        pthread_exit(NULL);
    }

    for(int i = 0;i < rows;i++){
        for(int j = 0;j < columns;j++){
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

    for (int i = 0;i < rows;i++){
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
    int** board = (int**)malloc(rows * sizeof(int*));
    for (int i = 0;i < rows;i++){
        board[i] = (int*)malloc(columns * sizeof(int));
        memcpy(board[i],data->board[i],columns * sizeof(int));
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
    for (int i = 0;i < rows;i++){
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
    copy->board = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        copy->board[i] = (int*)malloc(columns * sizeof(int));
        memcpy(copy->board[i], data->board[i], columns * sizeof(int));
    }
    return copy;
}


int* calculate(int color, int** board){
    Data2* data = (Data2*)malloc(sizeof(Data2));
    data->color = color;
    data->board = (int**)malloc(rows * sizeof(int*));
    for (int i = 0;i < rows;i++){
        data->board[i] = (int*)malloc(columns * sizeof(int));
        memcpy(data->board[i],board[i],columns * sizeof(int));
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
void* search(void *arg){
    Data* data = (Data*)arg;
    if (data->step == 0){
        int* result = (int*)calculate(2,data->board); // HERE burayı bi tempe at tempi resulta at tempi freele
              
        
        return (void*)result;
    }

    int length = 0, info1, info2, a, b;
    int* maximum = (int*)malloc(sizeof(int));
    *maximum = -1000000;
    Data** datas = (Data**)malloc(directionsize * sizeof(Data*));
    int** array;
    int* result;

    result = (int*)malloc(2 * sizeof(int));
    array = (int**)malloc(sizeof(int*));
    if (data->not_x > -1 && data->not_y > -1){
        info1 = newnode[data->not_x][data->not_y]->frame;
    }else{
        info1 = -1;
    }
    info2 = newnode[data->x][data->y]->frame;
    data->board[data->x][data->y] = data->color;

    if (data->color == 2){
        data->color = 1;
    }else if (data->color == 1){
        data->color = 2;
    }else{
        printf("COLOR ERROR %d",data->color);
    }

    for (int k = 0;k < directionsize;k++){
        if ((data->x + directions[k][0] != data->not_x ||
            data->y + directions[k][1] != data->not_y) &&
            data->x + directions[k][0] < rows &&
            data->x + directions[k][0] > -1 &&
            data->y + directions[k][1] < columns &&
            data->y + directions[k][1] > -1 &&
            data->board[data->x + directions[k][0]][data->y + directions[k][1]] == 0 &&
            newnode[data->x + directions[k][0]][data->y + directions[k][1]]->frame != info1 &&
            newnode[data->x + directions[k][0]][data->y + directions[k][1]]->frame != info2){

            length++;
            
            data->board[data->x + directions[k][0]][data->y + directions[k][1]] = data->color;
            array = (int**)realloc(array,length * sizeof(int*));
            array[length-1] = (int*)malloc(3 * sizeof(int));
            datas[k] = (Data*)malloc(sizeof(Data));
            datas[k]->x = data->x + directions[k][0];
            datas[k]->y = data->y + directions[k][1];
            datas[k]->step = data->step - 1;
            datas[k]->not_x = data->x;
            datas[k]->not_y = data->y;
            datas[k]->color = data->color;
            datas[k]->ret = false;
            datas[k]->board = (int**)malloc(rows * sizeof(int*));
            for (int i = 0; i < rows; i++) {
                datas[k]->board[i] = (int*)malloc(columns * sizeof(int));
                memcpy(datas[k]->board[i], data->board[i], columns * sizeof(int));
            }

            
            int* temppp = (int*)malloc(sizeof(int));
            temppp = search((void*)datas[k]);
            
            array[length-1][0] = *temppp;
            free(temppp); // HERE
            
            freedata(datas[k]);
            free(datas[k]);

            array[length-1][1] = data->x + directions[k][0];
            array[length-1][2] = data->y + directions[k][1];
            data->board[data->x + directions[k][0]][data->y + directions[k][1]] = 0;
        }
    }

    for(int i = 0;i < length;i++){
        if (array[i][0] > *maximum){
            *maximum = array[i][0];
            result[0] = array[i][1];
            result[1] = array[i][2];
        }
        free(array[i]);
    }
    free(array);
    free(datas);
    
    
    if (data->ret){
        //freedata(data);
        //free(data);
        return (void*)result;
    }
    free(result); // HERE
    
    return (void*)maximum;
}

int* best_place(int x, int y,int step, int lx, int ly){
    int i, j;
    printf("X: %d Y: %d\n",x,y);
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

    printf("X: %d Y: %d\n",x,y);
    int* temp;
    Data data;
    data.x = x;
    data.y = y;
    data.step = step;
    data.not_x = lx;
    data.not_y = ly;
    data.color = 2;
    data.ret = true;
    int** board = (int**)malloc(rows * sizeof(int*));
    for (i = 0;i < rows;i++){
        board[i] = (int*)malloc(columns * sizeof(int));
        memcpy(board[i],board2[i], columns * sizeof(int));
    }
    
    data.board = board;
    
    
    printf("X: %d Y: %d\n",x,y);
    file = fopen("data.txt","a");
    for(int i = 0;i < rows;i++){
        for(int j = 0;j < columns;j++){
            fprintf(file,"%d ",board2[i][j]);
        }
    }
    fprintf(file, "---> ");
    
    temp = (int*)search((void*)&data);
    fprintf(file,"(%d,%d)\n",temp[0],temp[1]);
    fclose(file);
    printf("X: %d Y: %d\n",x,y);
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
    
    ones = (int**)malloc(sizeof(int*));
    board2 = (int**)malloc(rows * sizeof(int*));
    for(int i = 0;i < rows;i++){
        board2[i] = (int*)malloc(columns * sizeof(int));
    }
    // make everywhere empty
    for (int i = 0;i < rows; i++){
        for (int j = 0;j < columns;j++){
            board2[i][j] = 0;
        }
    }
    // make checked array empty
    checked = (bool**)malloc(rows * sizeof(bool*));
    for(int i = 0;i < rows;i++){
        checked[i] = (bool*)malloc(rows * sizeof(bool));
        for (int j = 0;j < rows;j++){
            checked[i][j] = false;
        }
    }

    for (int i = 0;i < rows;i++){
        for(int j = 0;j < columns;j++){
            newnode[i][j] = (Node*)malloc(sizeof(Node));
            newnode[i][j]->frame = which(i,j);
        }
    }
    return 0;
}
