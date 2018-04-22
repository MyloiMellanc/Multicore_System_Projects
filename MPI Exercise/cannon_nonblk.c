#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


int comm_size;
int comm_rank;

unsigned int* temp_arr;

int axis_a = 0;
int axis_b = 0;

int axis_x,axis_y;
int x_range,y_range;
int x_rank_num, y_rank_num;

int send_a_rank;
int send_b_rank;
int recv_a_rank;
int recv_b_rank;

int x_start, x_end, y_start, y_end;

MPI_Status status;

MPI_Request request_1 = MPI_REQUEST_NULL;
MPI_Request request_2 = MPI_REQUEST_NULL;


void setCommunication() {
    axis_x = comm_rank / y_rank_num;
    
    send_a_rank = comm_rank - 1;
    if(send_a_rank < axis_x * y_rank_num)
        send_a_rank += y_rank_num;
    
    recv_a_rank = comm_rank + 1;
    if(recv_a_rank >= (axis_x + 1) * y_rank_num)
        recv_a_rank -= y_rank_num;
    
    axis_y = comm_rank % y_rank_num;
    
    send_b_rank = comm_rank - y_rank_num;
    if(send_b_rank < 0)
        send_b_rank += comm_size;
    
    recv_b_rank = comm_rank + y_rank_num;
    if(recv_b_rank >= comm_size)
        recv_b_rank -= comm_size;
    
    x_start = axis_x * x_range;
    x_end = x_start + x_range - 1;
    y_start = axis_y * y_range;
    y_end = y_start + y_range - 1;
}


void initCannonMatrix(int n, unsigned int* a, unsigned int* b, MPI_Comm* comm_x, MPI_Comm* comm_y) {
    for(int i = 0; i < n; i++)
        temp_arr[i] = 0;
    
    int temp_pos;
    int temp_line = 0;
    int temp_line_next = 1;
    int temp_line_back = 2;
    
    //A
    for(int x = x_start; x <= x_end; x++) {
        for(int y = y_start; y <= y_end; y++) {
            temp_pos = y - x;
            if(temp_pos < 0)
                temp_pos += n;
            
            temp_arr[temp_line * n + temp_pos] = a[x * n + y];
        }
        
        //ALL REDUCE
        MPI_Iallreduce(&temp_arr[temp_line * n], &temp_arr[temp_line_back * n], n, MPI_UNSIGNED, MPI_SUM, *comm_x, &request_1);
        
        for(int i = 0; i < n; i++)
            temp_arr[temp_line_next * n + i] = 0;
        
        MPI_Wait(&request_1, &status);
        
        for(int y = y_start; y <= y_end; y++) {
            a[x * n + y] = temp_arr[temp_line_back * n + y];
        }
        
        temp_line = (temp_line + 1) % 3;
        temp_line_next = (temp_line_next + 1) % 3;
        temp_line_back = (temp_line_back + 1) % 3;
    }
    
    
    //B
    for(int y = y_start; y <= y_end; y++) {
        for(int x = x_start; x <= x_end; x++) {
            temp_pos = x - y;
            if(temp_pos < 0)
                temp_pos += n;
            temp_arr[temp_line * n + temp_pos] = b[x * n + y];
        }
        
        //ALL REDUCE
        MPI_Iallreduce(&temp_arr[temp_line * n], &temp_arr[temp_line_back * n], n, MPI_UNSIGNED, MPI_SUM, *comm_y, &request_1);
        
        for(int i = 0; i < n; i++)
            temp_arr[temp_line_next * n + i] = 0;
        
        MPI_Wait(&request_1, &status);
        
        for(int x = x_start; x <= x_end; x++) {
            b[x * n + y] = temp_arr[temp_line_back * n + x];
        }
        
        temp_line = (temp_line + 1) % 3;
        temp_line_next = (temp_line_next + 1) % 3;
        temp_line_back = (temp_line_back + 1) % 3;
    }
}


void endCannonMatrix(int n, unsigned int* a, unsigned int* b, MPI_Comm* comm_x, MPI_Comm* comm_y) {
    for(int i = 0; i < n; i++)
        temp_arr[i] = 0;
    
    int temp_pos;
    int temp_line = 0;
    int temp_line_next = 1;
    int temp_line_back = 2;
    
    //A
    for(int x = x_start; x <= x_end; x++) {
        for(int y = y_start; y <= y_end; y++) {
            temp_pos = y + x;
            if(temp_pos >= n)
                temp_pos -= n;
            
            temp_arr[temp_line * n + temp_pos] = a[x * n + y];
        }
        
        //ALL REDUCE
        MPI_Iallreduce(&temp_arr[temp_line * n], &temp_arr[temp_line_back * n], n, MPI_UNSIGNED, MPI_SUM, *comm_x, &request_1);
        
        for(int i = 0; i < n; i++)
            temp_arr[temp_line_next * n + i] = 0;
        
        MPI_Wait(&request_1, &status);
        
        for(int y = y_start; y <= y_end; y++) {
            a[x * n + y] = temp_arr[temp_line_back * n + y];
        }
        
        temp_line = (temp_line + 1) % 3;
        temp_line_next = (temp_line_next + 1) % 3;
        temp_line_back = (temp_line_back + 1) % 3;
    }
    
    
    //B
    for(int y = y_start; y <= y_end; y++) {
        for(int x = x_start; x <= x_end; x++) {
            temp_pos = x + y;
            if(temp_pos >= n)
                temp_pos -= n;
            temp_arr[temp_line * n + temp_pos] = b[x * n + y];
        }
        
        //ALL REDUCE
        MPI_Iallreduce(&temp_arr[temp_line * n], &temp_arr[temp_line_back * n], n, MPI_UNSIGNED, MPI_SUM, *comm_y, &request_1);
        
        for(int i = 0; i < n; i++)
            temp_arr[temp_line_next * n + i] = 0;
        
        MPI_Wait(&request_1, &status);
        
        for(int x = x_start; x <= x_end; x++) {
            b[x * n + y] = temp_arr[temp_line_back * n + x];
        }
        
        temp_line = (temp_line + 1) % 3;
        temp_line_next = (temp_line_next + 1) % 3;
        temp_line_back = (temp_line_back + 1) % 3;
    }
}


void shiftMatrix(int n, unsigned int* a, unsigned int* b) {
    MPI_Irecv(&temp_arr[x_range], x_range, MPI_UNSIGNED, recv_a_rank, 0, MPI_COMM_WORLD, &request_1);
    
    for(int i = 0; i < x_range; i++) {
        temp_arr[i] = a[(x_start + i) * n + y_start + axis_a];
    }
    
    MPI_Isend(temp_arr, x_range, MPI_UNSIGNED, send_a_rank, 0, MPI_COMM_WORLD, &request_2);
    
    MPI_Wait(&request_1,&status);
    
    for(int i = 0; i < x_range; i++) {
        a[(x_start + i) * n + y_start + axis_a] = temp_arr[x_range + i];
    }
    
    for(int i = 0; i < y_range; i++) {
        temp_arr[n + i] = b[(x_start + axis_b) * n + y_start + i];
    }
    
    
    MPI_Irecv(&b[(x_start + axis_b) * n + y_start], y_range, MPI_UNSIGNED, recv_b_rank, 0, MPI_COMM_WORLD, &request_1);
    
    MPI_Wait(&request_2, &status);
    
    MPI_Isend(&temp_arr[n], y_range, MPI_UNSIGNED, send_b_rank, 0, MPI_COMM_WORLD, &request_2);
    
    
    MPI_Wait(&request_1,&status);
    
    MPI_Wait(&request_2,&status);
    
    axis_a++;
    if(axis_a >= y_range)
        axis_a -= y_range;
    axis_b++;
    if(axis_b >= x_range)
        axis_b -= x_range;
}


void MatrixMatrixMultiply(int n, unsigned int *a, unsigned int *b, unsigned int *c) {
    MPI_Init(NULL, NULL);
    
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    
    
    if(comm_size == 4) {
        x_rank_num = 2;
        y_rank_num = 2;
        x_range = n / x_rank_num;
        y_range = n / y_rank_num;
    }
    else if(comm_size == 8) {
        x_rank_num = 2;
        y_rank_num = 4;
        x_range = n / x_rank_num;
        y_range = n / y_rank_num;
    }
    else if(comm_size == 16) {
        x_rank_num = 4;
        y_rank_num = 4;
        x_range = n / x_rank_num;
        y_range = n / y_rank_num;
    }
    
    setCommunication();
    
    int rank_color_x = comm_rank / y_rank_num;
    int rank_color_y = comm_rank % y_rank_num;
    MPI_Comm comm_x;
    MPI_Comm comm_y;
    MPI_Comm_split(MPI_COMM_WORLD, rank_color_x, comm_rank, &comm_x);
    MPI_Comm_split(MPI_COMM_WORLD, rank_color_y, comm_rank, &comm_y);
    
    
    temp_arr = (unsigned int*)malloc(sizeof(unsigned int) * n * 3);
    
    initCannonMatrix(n, a, b, &comm_x, &comm_y);
    
    
    //MPI_Barrier(MPI_COMM_WORLD);
    
    
    int X,Y;
    for(int i = 0; i < n; i++) {
        for(int x = x_start; x <= x_end; x++) {
            X = x + axis_b;
            if(X > x_end)
                X -= x_range;
            for(int y = y_start; y <= y_end; y++) {
                Y = y + axis_a;
                if(Y > y_end)
                    Y -= y_range;
                c[x * n + y] += a[x * n + Y] * b[X * n + y];
            }
        }
        
        shiftMatrix(n, a, b);
    }
    
    endCannonMatrix(n, a, b, &comm_x, &comm_y);
    
    free(temp_arr);

    unsigned int* d;
    
    if(comm_rank == 0) {
        d = (unsigned int*)malloc(sizeof(unsigned int) * n * n);
        MPI_Ireduce(c, d, n*n, MPI_UNSIGNED, MPI_MAX, 0, MPI_COMM_WORLD, &request_1);
        
        MPI_Wait(&request_1, &status);
        
        for(int i = 0; i < n * n; i++) {
            c[i] = d[i];
        }
        
        free(d);
    }
    else {
        MPI_Ireduce(c, d, n*n, MPI_UNSIGNED, MPI_MAX, 0, MPI_COMM_WORLD, &request_1);
        MPI_Comm_free(&comm_x);
        MPI_Comm_free(&comm_y);
        
        free(a);
        free(b);
        
        MPI_Wait(&request_1, &status);
        free(c);
        MPI_Finalize();
        exit(0);
    }
    MPI_Comm_free(&comm_x);
    MPI_Comm_free(&comm_y);
    
    MPI_Finalize();
}






