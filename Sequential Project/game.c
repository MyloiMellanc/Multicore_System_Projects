
#include "game.h"


void make_cubic(struct setup* s){
	int number = s->map_size;

	if ((cubic = (cell***)malloc(number * sizeof(cell**))) == NULL) {
		exit(1);
	}

	for (int i = 0; i < number; i++) {
		if ((cubic[i] = (cell**)malloc(number * sizeof(cell*))) == NULL) {
			exit(1);
		}
	}

	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			if ((cubic[i][j] = (cell*)malloc(number * sizeof(cell))) == NULL) {
				exit(1);
			}
		}
	}
}


void init_cubic(struct setup *s){
	int number = s->map_size;
	int live_seed = s->SEED_MAP;
    
	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
                //Fill total cell state to LIVE or DEAD by using uniform()
				short lived = uniform(0, 1, live_seed);
                
				cubic[i][j][k].state = lived;
				cubic[i][j][k].next_prev_state = NONE;
				cubic[i][j][k].on_devil = 0;
				cubic[i][j][k].next_devil = false;
			}
		}
	}
	
	
}

void init_angel(struct setup* s) {
	//set Angel Position
	cubic_angel = (angel*)malloc(sizeof(angel));
    
	double angel_pos_double = (double)s->map_size / 2;
	int angel_pos = s->map_size / 2;

	if(angel_pos_double - (double)angel_pos >= 0.5)
		angel_pos++;

	angel_pos--;
	
	cubic_angel->x = angel_pos;
	cubic_angel->y = angel_pos;
	cubic_angel->z = angel_pos;
}

void init_devil(struct setup* s) {
	//set Devil Position
	deploy_devil(s, 1);
	spread_plague(s);
}


void deploy_devil(struct setup* s, int number) {
    //Add devil count into cubic
	for (int i = 0; i < number; i++) {
		int devil_x = uniform(0, s->map_size - 1, s->SEED_DVL_GEN_X);
		int devil_y = uniform(0, s->map_size - 1, s->SEED_DVL_GEN_Y);
		int devil_z = uniform(0, s->map_size - 1, s->SEED_DVL_GEN_Z);

		cubic[devil_x][devil_y][devil_z].on_devil++;
	}
}

void move_all_devils(struct setup* s, int x, int y, int z) {
	int number = s->map_size;
	int move_x, move_y, move_z;

	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				if (cubic[i][j][k].on_devil > 0) {
                    //check boundary of cubic
					if ((i + x >= 0) && (i + x < number))
						move_x = x;
					else
						move_x = 0;

					if ((j + y >= 0) && (j + y < number))
						move_y = y;
					else
						move_y = 0;

					if ((k + z >= 0) && (k + z < number))
						move_z = z;
					else
						move_z = 0;

					cubic[i][j][k].on_devil = 0;
					cubic[i + move_x][j + move_y][k + move_z].next_devil = true;
				}
			}
		}
	}

	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				if (cubic[i][j][k].next_devil == true)
					cubic[i][j][k].on_devil = 1;

				cubic[i][j][k].next_devil = false;
			}
		}
	}
}

void spread_plague(struct setup* s) {
	int number = s->map_size;

	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
                //set plague into cell where devil exists
				if (cubic[i][j][k].on_devil > 0)
					set_cell_plagued(i, j, k);
			}
		}
	}
}

void set_cell_plagued(int x, int y, int z) {
	if (cubic[x][y][z].state != PLAGUE) {
        //save previous state of cell
		cubic[x][y][z].next_prev_state = cubic[x][y][z].state;
		cubic[x][y][z].state = PLAGUE;
	}
}

int get_devil_count(struct setup* s) {
	int number = s->map_size;
	int devil_count = 0;
    
	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				devil_count += cubic[i][j][k].on_devil;
			}
		}
	}

	return devil_count;
}



void update_cell_toggle_plagued(struct setup* s, int x, int y, int z) {
    //change cell state if plague exists somewhere
	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			for (int k = -1; k < 2; k++) {
				cell* adjust_cell = get_cell(s, x + i, y + j, z + k);
				if ((adjust_cell != NULL) && (adjust_cell->state == PLAGUE))
					if (cubic[x][y][z].state == LIVE) {
						cubic[x][y][z].state = DEAD;
						return;
					}
					else if (cubic[x][y][z].state == DEAD) {
						cubic[x][y][z].state = LIVE;
						return;
					}	
			}
		}
	}
}


void update_cell_toggle(struct setup* s, int x, int y, int z) {
    //change state by adjust state of cells
	short current_state = cubic[x][y][z].state;
	int live_count = 0;

	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			for (int k = -1; k < 2; k++) {
				if ((i != 0) || (j != 0) || (k != 0)) {
					cell* adjust_cell = get_cell(s, x + i, y + j, z + k);
					if ((adjust_cell != NULL) && (adjust_cell->state != PLAGUE)) {
						if (adjust_cell->state == LIVE)
							live_count++;
					}
				}
			}
		}
	}
	
	if (current_state == DEAD)
		if ((live_count > s->live_min) && (live_count < s->live_max))
			cubic[x][y][z].next_prev_state = LIVE;
		else
			cubic[x][y][z].next_prev_state = DEAD;

	else if (current_state == LIVE)
		if ((live_count < s->dead_min) || (live_count > s->dead_max))
			cubic[x][y][z].next_prev_state = DEAD;
		else
			cubic[x][y][z].next_prev_state = LIVE;
}


cell* get_cell(struct setup* s, int x, int y, int z) {
	int number = s->map_size;

	if ((x >= 0) && (y >= 0) && (z >= 0))
		if ((x < number) && (y < number) && (z < number))
			return &cubic[x][y][z];

    //if don't exists, return nullptr
	return NULL;
}


//function for setting direction of angel
int get_devil_count_in_submatrix(int min_x, int max_x, int min_y, int max_y, int min_z, int max_z) {
	int devil_count = 0;

    //count the number of devil in subrange
	for (int x = min_x; x <= max_x; x++) {
		for (int y = min_y; y <= max_y; y++) {
			for (int z = min_z; z <= max_z; z++) {
					devil_count += cubic[x][y][z].on_devil;
			}
		}
	}

	return devil_count;
}


angel set_angel_direction(int* devil_count_arr) {
	angel direction;
	direction.x = 0;
	direction.y = 0;
	direction.z = 0;

	int max = 0;
	for (int i = 0; i < 6; i++) {
		if (max < devil_count_arr[i])
			max = devil_count_arr[i];
	}

	int devil_direction = 7;
	for (int i = 0; i < 6; i++) {
		if (devil_count_arr[i] == max) {
			devil_direction = i;
			break;
		}
	}

	if (devil_direction == 7) {
		//directing error
		exit(1);
	}

	switch (devil_direction) {
	case PLUS_X:
		direction.x = 1;
		break;
	case PLUS_Y:
		direction.y = 1;
		break;
	case PLUS_Z:
		direction.z = 1;
		break;
	case MINUS_X:
		direction.x = -1;
		break;
	case MINUS_Y:
		direction.y = -1;
		break;
	case MINUS_Z:
		direction.z = -1;
		break;
	default:
		break;
	}

	return direction;
}


int move_angel(struct setup* s, int* angel_direction) {
	int number = s->map_size;
	int before[3] = { cubic_angel->x, cubic_angel->y, cubic_angel->z };

	cubic_angel->x = cubic_angel->x + angel_direction[0];
	if (cubic_angel->x < 0)
		cubic_angel->x = 0;
	else if (cubic_angel->x >= number)
		cubic_angel->x = number - 1;

	cubic_angel->y = cubic_angel->y + angel_direction[1];
	if (cubic_angel->y < 0)
		cubic_angel->y = 0;
	else if (cubic_angel->y >= number)
		cubic_angel->y = number - 1;

	cubic_angel->z = cubic_angel->z + angel_direction[2];
	if (cubic_angel->z < 0)
		cubic_angel->z = 0;
	else if (cubic_angel->z >= number)
		cubic_angel->z = number - 1;

	int after[3] = { cubic_angel->x, cubic_angel->y, cubic_angel->z };

	if (after[0] - before[0] != 0)
		return abs(after[0] - before[0]);
	else if (after[1] - before[1] != 0)
		return abs(after[1] - before[1]);
	else if (after[2] - before[2] != 0)
		return abs(after[2] - before[2]);
	
	return number / 10;


}

void purify_devil_plague(struct setup* s, int angel_scope) {
	int number = s->map_size;

	int min_x = cubic_angel->x - angel_scope;
	if (min_x < 0)
		min_x = 0;
	int max_x = cubic_angel->x + angel_scope;
	if (max_x >= number)
		max_x = number - 1;

	int min_y = cubic_angel->y - angel_scope;
	if (min_y < 0)
		min_y = 0;
	int max_y = cubic_angel->y + angel_scope;
	if (max_y >= number)
		max_y = number - 1;

	int min_z = cubic_angel->z - angel_scope;
	if (min_z < 0)
		min_z = 0;
	int max_z = cubic_angel->z + angel_scope;
	if (max_z >= number)
		max_z = number - 1;

	for (int x = min_x; x <= max_x; x++) {
		for (int y = min_y; y <= max_y; y++) {
			for (int z = min_z; z <= max_z; z++) {
				if (cubic[x][y][z].on_devil > 0)
					cubic[x][y][z].on_devil = 0;
				if (cubic[x][y][z].state == PLAGUE) {
					cubic[x][y][z].state = cubic[x][y][z].next_prev_state;
					cubic[x][y][z].next_prev_state = NONE;
				}
			}
		}
	}
}




//////////////////////////////////////////////////////////////

void init_resources(struct setup *s) {
	make_cubic(s);
	init_cubic(s);
	init_angel(s);
	init_devil(s);
}

void devil_stage(struct setup *s) {
	//if devil count is 0, create a new devil
	if (get_devil_count(s) == 0)
		deploy_devil(s, 1);

	//make direction for all devils.
	int move[3];
	move[0] = uniform(0, 2, s->SEED_DVL_MOV_X);
	move[1] = uniform(0, 2, s->SEED_DVL_MOV_Y);
	move[2] = uniform(0, 2, s->SEED_DVL_MOV_Z);

	for (int i = 0; i < 3; i++)
	{
		if (move[i] == 0)
			move[i] = 1;
		else if (move[i] == 1)
			move[i] = 0;
		else if (move[i] == 2)
			move[i] = -1;
	}

	//move all devils.
	move_all_devils(s, move[0], move[1], move[2]);

	//devils would plague cell.
	spread_plague(s);

	//copy devil.
	int devil_count = get_devil_count(s);
	deploy_devil(s, devil_count);

}

void live_dead_stage(struct setup *s) {
	int number = s->map_size;

	for(int i = 0; i < number; i++){
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				if(cubic[i][j][k].state != PLAGUE)
                    //change next_prev_state of cell by adjust cells
					update_cell_toggle(s, i, j, k);
			}
		}
	}

	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				if (cubic[i][j][k].state != PLAGUE) {
                    //update cell by next_prev_state
					cubic[i][j][k].state = cubic[i][j][k].next_prev_state;
					cubic[i][j][k].next_prev_state = NONE;
				}
			}
		}
	}
}

void plague_stage (struct setup *s) {
	int number = s->map_size;

	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				if (cubic[i][j][k].state != PLAGUE)
					update_cell_toggle_plagued(s, i, j, k);
			}
		}
	}
}

void angel_stage (struct setup *s) {
	int number = s->map_size;

	//set direction, move value
	int devil_count[6];
	devil_count[PLUS_X] = get_devil_count_in_submatrix(cubic_angel->x + 1, number - 1, 0, number - 1, 0, number - 1);
	devil_count[PLUS_Y] = get_devil_count_in_submatrix(0, number - 1, cubic_angel->y + 1, number - 1, 0, number - 1);
	devil_count[PLUS_Z] = get_devil_count_in_submatrix(0, number - 1, 0, number - 1, cubic_angel->z + 1, number - 1);
	devil_count[MINUS_X] = get_devil_count_in_submatrix(0, cubic_angel->x - 1, 0, number - 1, 0, number - 1);
	devil_count[MINUS_Y] = get_devil_count_in_submatrix(0, number - 1, 0, cubic_angel->y - 1, 0, number - 1);
	devil_count[MINUS_Z] = get_devil_count_in_submatrix(0, number - 1, 0, number - 1, 0, cubic_angel->z - 1);

    //set direction by devil count array
	angel angel_direction = set_angel_direction(devil_count);

	int move_value = (int)(number / 10);

	int angel_move[3] = { 0,0,0 };
	angel_move[0] = angel_direction.x * move_value;
	angel_move[1] = angel_direction.y * move_value;
	angel_move[2] = angel_direction.z * move_value;

	//move and return real move value
	move_value = move_angel(s, angel_move);

	
	//set scope
	int angel_scope = (int)(get_devil_count(s) / (5 * number * number));
	int X = (int)(move_value / 2);

	if (X > angel_scope)
		angel_scope = X;


	//purify
	purify_devil_plague(s, angel_scope);


}



void print_init_map (struct setup *s) {
	FILE *f;
	f = fopen("Initial_map.txt", "w");
	if (f == NULL) {
		exit(1);
	}

	int number = s->map_size;

	for (int x = 0; x < number; x++) {
		for (int y = 0; y < number; y++) {
			for (int z = 0; z < number; z++) {
				if (cubic[x][y][z].state == DEAD)
					fprintf(f, "D ");
				else if (cubic[x][y][z].state == LIVE)
					fprintf(f, "L ");
				else if (cubic[x][y][z].state == PLAGUE)
					fprintf(f, "P ");
			}
			fprintf(f, "\n");
		}
		fprintf(f, "\n");
	}

	fclose(f);

}

void print_init_pos (struct setup *s) {
	FILE* f;
	f = fopen("Initial_pos.txt", "w");
	if (f == NULL) {
		exit(1);
	}
	fprintf(f, "[Angel]\n");
	fprintf(f, "(%d, %d, %d)\n\n", cubic_angel->x, cubic_angel->y, cubic_angel->z);

	int number = s->map_size;
	int x, y, z;
	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				if (cubic[i][j][k].on_devil > 0) {
					x = i;
					y = j;
					z = k;

					break;
				}
			}
		}
	}

	fprintf(f, "[Devil]\n");
	fprintf(f, "(%d, %d, %d)\n", x, y, z);

	fclose(f);

}

void print_fin_map (struct setup *s) {
	FILE *f;
	f = fopen("Final_map.txt", "w");
	if (f == NULL) {
		exit(1);
	}

	int number = s->map_size;

	for (int x = 0; x < number; x++) {
		for (int y = 0; y < number; y++) {
			for (int z = 0; z < number; z++) {
				if (cubic[x][y][z].state == DEAD)
					fprintf(f, "D ");
				else if (cubic[x][y][z].state == LIVE)
					fprintf(f, "L ");
				else if (cubic[x][y][z].state == PLAGUE)
					fprintf(f, "P ");
			}
			fprintf(f, "\n");
		}
		fprintf(f, "\n");
	}

	fclose(f);

}

void print_fin_pos (struct setup *s) {
	FILE* f;
	f = fopen("Final_pos.txt", "w");
	if (f == NULL) {
		exit(1);
	}
	fprintf(f, "[Angel]\n");
	fprintf(f, "(%d, %d, %d)\n\n", cubic_angel->x, cubic_angel->y, cubic_angel->z);

	int number = s->map_size;

	fprintf(f, "[Devil]\n");

	for (int x = 0; x < number; x++) {
		for (int y = 0; y < number; y++) {
			for (int z = 0; z < number; z++) {
				while (cubic[x][y][z].on_devil > 0) {
					fprintf(f, "(%d, %d, %d)\n", x, y, z);
					cubic[x][y][z].on_devil--;
				}
			}
		}
	}

	fclose(f);

}

void free_resources (struct setup *s) {
	free(cubic);
	free(cubic_angel);
}
