#ifndef __LIB_GAME_H__
#define __LIB_GAME_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>

#include <mpi.h>

#include "lcgrand.h"
#include "setup.h"


#define LIVE 1
#define DEAD 0
#define PLAGUE 2
#define NONE 3

struct setup s;
int map_size;

typedef struct Cell
{
    unsigned char state;
    unsigned char next_prev_state;
}cell;

cell* cubic;
MPI_Datatype cubic_type;

short* devil_cubic;
//short* devil_temp;

typedef struct Vec3
{
    int x;
    int y;
    int z;
}vec3;

vec3 angel;

vec3 devil_move;

//Direction of Angel
#define PLUS_X 0
#define PLUS_Y 1
#define PLUS_Z 2
#define MINUS_X 3
#define MINUS_Y 4
#define MINUS_Z 5


//variable for each MPI process

unsigned long angel_sight[6];
unsigned long devil_count;
unsigned long corpse;

MPI_Request request_1;
MPI_Request request_2;

MPI_Status status;

int toggle;

int comm_size;
int comm_rank;

int x_position;
int x_range;


int x_min;  //for transfer
int x_start;      //for loop
int x_end;    //for loop
int x_max;  //for transfer


///////////////////////////////////

void init_angel_sight();
void set_cell_plagued(int x, int y, int z);
void check_angel_sight(int x, int y, int z, int devil_num);
void move_devils();
void spread_plague();
cell* get_cell(int x, int y, int z);
void stack_initialize(char* cube_stack, char* stack_pointer, char* cube_state);
void update_cell_toggle_and_plague(char* cube_stack, char* stack_pointer, char* cube_state, int x, int y, int z);

vec3 set_angel_direction(unsigned long* angel_sight);
int move_angel(int* angel_direction);
void purify_devil_plague(int angel_scope);

///////////////////////////////////


//Added init funcs
void make_cubic();
void init_cubic();
void init_angel();
void init_devil();
void init_thread();




void devil_stage();
void live_dead_plague_stage();
void angel_stage();


void init_resources();
void run_game();
void print_init_map();
void print_init_pos();
void print_fin_map();
void print_fin_pos();
void free_resources();

#endif



