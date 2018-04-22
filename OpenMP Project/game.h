#ifndef __LIB_GAME_H__
#define __LIB_GAME_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>


#include "lcgrand.h"
#include "setup.h"


#define LIVE 1
#define DEAD 0
#define PLAGUE 2
#define NONE 3

struct setup* _s;
int number;	//for map_size

typedef struct Cell
{
	unsigned char state : 4;
	unsigned char next_prev_state : 4;
}cell;

typedef struct Devil_Cell {
	short on_devil;
	bool next_devil;
}devil_cell;

//3-Dimention array of cells;
cell* cubic;

devil_cell* devil_cubic;


//Position of Angel
typedef struct Vec3
{
	int x;
	int y;
	int z;
}vec3;

vec3* cubic_angel;

//Direction of Angel
#define PLUS_X 0
#define PLUS_Y 1
#define PLUS_Z 2
#define MINUS_X 3
#define MINUS_Y 4
#define MINUS_Z 5


//Added init funcs
void make_cubic();
void init_cubic();
void init_angel();
void init_devil();


//func for devil
void deploy_devil(int number);
void move_all_devils_and_spread_plague();
void set_cell_plagued(int x, int y, int z);
int get_devil_count();
devil_cell* get_devil_cell(int x, int y, int z);

//func for control cell state
cell* get_cell(int x, int y, int z);
void update_cell_toggle_and_plague(char* cube_stack, char* stack_pointer, char* cube_state, int x, int y, int z);

//func for angel
int get_devil_count_in_submatrix(int min_x, int max_x, int min_y, int max_y, int min_z, int max_z);
vec3 set_angel_direction(int* devil_count_arr);
int move_angel(int* angel_direction);
void purify_devil_plague(int angel_scope);

////////////////Thread/////////////////////
typedef struct Thread_Range{
	short x_start;
	short x_end;
}thread_range;

thread_range* thread_range_list;

int thread_num;

//thread counter
int thread_toggle_counter;
int max_counter;
bool pass;

void init_thread_counter();
void toggle_thread_counter();

////////////////////////////

void init_thread();


vec3* devil_move;
int devil_count[6];
void init_devil_count();

void MP_move_devils_and_spread_plague();


void stack_initialize(char* cube_stack, char* stack_pointer, char* cube_state);
void MP_update_cell_state();




//Original funcs
void init_resources (struct setup *s);
void devil_live_plague_stage (struct setup *s);
void live_dead_stage (struct setup *s);
void plague_stage (struct setup *s);
void angel_stage (struct setup *s);
void print_init_map (struct setup *s);
void print_init_pos (struct setup *s);
void print_fin_map (struct setup *s);
void print_fin_pos (struct setup *s);
void free_resources (struct setup *s);
void run_game(struct setup* s);

#endif
