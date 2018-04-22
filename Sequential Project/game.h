#ifndef __LIB_GAME_H__
#define __LIB_GAME_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "lcgrand.h"
#include "setup.h"


#define LIVE 1
#define DEAD 0
#define PLAGUE 2
#define NONE 3

//Cell Struct for Cubic
typedef struct Cell
{
	short state;
	short next_prev_state;      //State after moving of devil
	short on_devil;
	bool next_devil;
}cell;


//3-Dimention array of cells;
cell*** cubic;

//Position of Angel
typedef struct Angel
{
	int x;
	int y;
	int z;
}angel;

//Main instance of Angel
angel* cubic_angel;

//Direction of Angel
#define PLUS_X 0
#define PLUS_Y 1
#define PLUS_Z 2
#define MINUS_X 3
#define MINUS_Y 4
#define MINUS_Z 5


//Added init funcs
void make_cubic(struct setup* s);
void init_cubic(struct setup* s);
void init_angel(struct setup* s);
void init_devil(struct setup* s);


//func for devil
void deploy_devil(struct setup* s, int number);
void move_all_devils(struct setup* s, int x, int y, int z);
void spread_plague(struct setup* s);
void set_cell_plagued(int x, int y, int z);
int get_devil_count(struct setup* s);

//func for activation of plague 
void update_cell_toggle_plagued(struct setup* s, int x, int y, int z);


//func for control cell state
cell* get_cell(struct setup* s, int x, int y, int z);
void update_cell_toggle(struct setup* s, int x, int y, int z);

//func for angel
int get_devil_count_in_submatrix(int min_x, int max_x, int min_y, int max_y, int min_z, int max_z);
angel set_angel_direction(int* devil_count_arr);
int move_angel(struct setup* s, int* angel_direction);
void purify_devil_plague(struct setup* s, int angel_scope);





//Original funcs
void init_resources (struct setup *s);
void devil_stage (struct setup *s);
void live_dead_stage (struct setup *s);
void plague_stage (struct setup *s);
void angel_stage (struct setup *s);
void print_init_map (struct setup *s);
void print_init_pos (struct setup *s);
void print_fin_map (struct setup *s);
void print_fin_pos (struct setup *s);
void free_resources (struct setup *s);

#endif
