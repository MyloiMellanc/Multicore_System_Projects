
#ifndef CANNON
#define CANNON

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


void initCommunication();
void initCannonMatrix(int n, unsigned int* a, unsigned int* b, MPI_Comm* comm_x, MPI_Comm* comm_y);
void endCannonMatrix(int n, unsigned int* a, unsigned int* b, MPI_Comm* comm_x, MPI_Comm* comm_y);
void shiftMatrix(int n, unsigned int* a, unsigned int* b);

void MatrixMatrixMultiply(int n, unsigned int *a, unsigned int *b, unsigned int *c);


#endif
