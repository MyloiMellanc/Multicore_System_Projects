
#include "game.h"


void make_cubic(){
	if ((cubic = (cell*)malloc(number * number * number * sizeof(cell))) == NULL) {
		printf("Cubic Allocation Failed.\n");
		exit(1);
	}

	
	if ((devil_cubic = (devil_cell*)malloc(number * number * number * sizeof(devil_cell))) == NULL) {
		printf("Devil Cubic Allocation Failed.\n");
		exit(1);
	}
}

void init_cubic(){
	//Fill total cell state to LIVE or DEAD by using uniform()
	int live_seed = _s->SEED_MAP;
    
	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				char lived = uniform(0, 1, live_seed);
				cubic[i * number * number + j * number + k].state = lived;
				cubic[i * number * number + j * number + k].next_prev_state = NONE;
			}
		}
	}
	
	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				devil_cubic[i * number * number + j * number + k].on_devil = 0;
				devil_cubic[i * number * number + j * number + k].next_devil = false;
			}
		}
	}
	
}

void init_angel() {
	//set Angel Position
	cubic_angel = (vec3*)malloc(sizeof(vec3));
	
	double angel_pos_double = (double)_s->map_size / 2.0f;
	int angel_pos = _s->map_size / 2;

	if (angel_pos_double - (double)angel_pos >= 0.5)
		angel_pos++;

	angel_pos--;

	cubic_angel->x = angel_pos;
	cubic_angel->y = angel_pos;
	cubic_angel->z = angel_pos;
}

void init_devil() {
	//set Devil Position
	int devil_x = uniform(0, _s->map_size - 1, _s->SEED_DVL_GEN_X);
	int devil_y = uniform(0, _s->map_size - 1, _s->SEED_DVL_GEN_Y);
	int devil_z = uniform(0, _s->map_size - 1, _s->SEED_DVL_GEN_Z);

	devil_cubic[devil_x * number * number + devil_y  * number + devil_z].on_devil++;
	set_cell_plagued(devil_x, devil_y, devil_z);

	devil_move = (vec3*)malloc(sizeof(vec3));
}

void init_thread() {
	int core_num = _s->core_num;
	thread_num = core_num;
	thread_range_list = (thread_range*)malloc(core_num * sizeof(thread_range));

	//int range_start = 0;
	int range_end = _s->map_size - 1;	//Because, Range start is zero.

	float range_per_thread = ((float)_s->map_size / (float)core_num);
	
	float x_position = 0;

	//loop except last one thread
	for (int i = 0; i < core_num - 1; i++) {
		thread_range_list[i].i = i;
		thread_range_list[i].x_start = (int)x_position;
		thread_range_list[i].x_end = (int)(x_position + range_per_thread - 1.0);

		x_position = x_position + range_per_thread;
	}

	//for last one
	thread_range_list[core_num - 1].i = core_num - 1;
	thread_range_list[core_num - 1].x_start = (int)x_position;
	thread_range_list[core_num - 1].x_end = range_end;


	init_thread_counter();
	max_counter = thread_num;
}

void init_thread_counter() {
	thread_toggle_counter = 0;
	pass = false;
}


//This function is dangerous
void toggle_thread_counter() {
    thread_toggle_counter++;    //it's dangerous becasuse it is not critical section
	if (thread_toggle_counter >= max_counter)
		pass = true;
}


void deploy_devil(int devil_num) {
	for (int i = 0; i < devil_num; i++) {
		int devil_x = uniform(0, _s->map_size - 1, _s->SEED_DVL_GEN_X);
		int devil_y = uniform(0, _s->map_size - 1, _s->SEED_DVL_GEN_Y);
		int devil_z = uniform(0, _s->map_size - 1, _s->SEED_DVL_GEN_Z);

		devil_cubic[devil_x * number * number + devil_y * number + devil_z].on_devil++;
	}
}

devil_cell* get_devil_cell(int x, int y, int z) {
	if ((x >= 0) && (y >= 0) && (z >= 0))
		if ((x < number) && (y < number) && (z < number))
			return &devil_cubic[x * number * number + y * number + z];

	return NULL;
}

void* PT_move_devils_and_spread_plague(void* thread_index) {
	int x = devil_move->x;
	int y = devil_move->y;
	int z = devil_move->z;

	int index = *((int*)thread_index);

	int x_start = thread_range_list[index].x_start;
	int x_end = thread_range_list[index].x_end;

	int move_x, move_y, move_z;

    
	for (int i = x_start; i <= x_end; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				if (devil_cubic[i * number * number + j * number + k].on_devil > 0) {
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

					
					get_devil_cell(i + move_x, j + move_y, k + move_z)->next_devil = true;

				}
			}
		}
	}

	//toggle and wait for pass trigger
	toggle_thread_counter();

	//if pass, work on.
	while (pass != true) {}

    
    
	for (int i = x_start; i <= x_end; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				devil_cubic[i * number * number + j * number + k].on_devil = 0;
				if (devil_cubic[i * number * number + j * number + k].next_devil == true) {
					devil_cubic[i * number * number + j * number + k].on_devil = 1;
					set_cell_plagued(i, j, k);
				}
				devil_cubic[i * number * number + j * number + k].next_devil = false;
			}
		}
	}

	return NULL;

}

void move_all_devils_and_spread_plague() {
    //Set threads and join them
	for (int i = 0; i < thread_num; i++) {
		pthread_create(&thread_range_list[i].id, NULL, PT_move_devils_and_spread_plague, (void*)&thread_range_list[i].i);
	}

	for (int i = 0; i < thread_num; i++) {
		pthread_join(thread_range_list[i].id, NULL);
	}
}



void set_cell_plagued(int x, int y, int z) {
	if (cubic[x * number * number + y * number + z].state != PLAGUE) {
		cubic[x * number * number + y * number + z].next_prev_state = cubic[x * number * number + y * number + z].state;
		cubic[x * number * number + y * number + z].state = PLAGUE;
	}
}

int get_devil_count() {
	int devil_count = 0;
	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				devil_count += devil_cubic[i * number * number + j * number + k].on_devil;
			}
		}
	}

	return devil_count;
}


//In Multithreading project, cubic saves previous adjust cell state and update it
//That stack data is used for live and dead stage and plague stage
//For playing both stage at the same time

void update_cell_toggle_and_plague(char* cube_stack, char* stack_pointer, char* cube_state, int x, int y, int z) {
	char current_state = get_cell(x, y, z)->state;

	for (int k = -1; k < 2; k++) {
		if (z != 0) k = 1;
		for (int i = -1; i < 2; i++) {
			for (int j = -1; j < 2; j++) {
				cell* adjust_cell = get_cell(x + i, y + j, z + k);
				if (adjust_cell != NULL) {
					cube_state[cube_stack[*stack_pointer]]--;
					cube_stack[*stack_pointer] = adjust_cell->state;
					cube_state[adjust_cell->state]++;
					*stack_pointer = (*stack_pointer + 1) % 27;
				}
				else {
					cube_state[cube_stack[*stack_pointer]]--;
					cube_stack[*stack_pointer] = DEAD;
					cube_state[DEAD]++;
					*stack_pointer = (*stack_pointer + 1) % 27;
				}
			}
		}
	}

	if (current_state == PLAGUE)
		return;

	int live_num = cube_state[LIVE];
	if (current_state == LIVE)
		live_num--;

	if (current_state == DEAD) {
		if ((live_num > _s->live_min) && (live_num  < _s->live_max))
			current_state = LIVE;
		else
			current_state = DEAD;
	}
	else if (current_state == LIVE) {
		if ((live_num < _s->dead_min) || (live_num > _s->dead_max))
			current_state = DEAD;
		else
			current_state = LIVE;
	}

	if (cube_state[PLAGUE] > 0) {
		if (current_state == LIVE)
			current_state = DEAD;
		else
			current_state = LIVE;
	}

	get_cell(x, y, z)->next_prev_state = current_state;
}


void stack_initialize(char* cube_stack, char* stack_pointer, char* cube_state) {
	for (int i = 0; i < 27; i++) {
		cube_stack[i] = DEAD;
	}
	*stack_pointer = 0;

	cube_state[DEAD] = 27;
	cube_state[LIVE] = 0;
	cube_state[PLAGUE] = 0;
}

void* PT_update_cell_state(void* thread_index) {
	int index = *((int*)thread_index);
	int x_start = thread_range_list[index].x_start;
	int x_end = thread_range_list[index].x_end;

	//Don't Control Cell which state is PLAGUE.

	char cube_stack[27];
	char stack_pointer;
	char cube_state[3];

	stack_initialize(cube_stack, &stack_pointer, cube_state);

	for (int x = x_start; x <= x_end; x++) {
		for (int y = 0; y < number; y++) {
			for (int z = 0; z < number; z++) {
					update_cell_toggle_and_plague(cube_stack, &stack_pointer, cube_state, x, y, z);
			}

			stack_initialize(cube_stack, &stack_pointer, cube_state);
		}
	}

    
    
    //toggle and wait for pass trigger
    toggle_thread_counter();

    //if pass, work on.
    while (pass != true) {}


	for (int x = x_start; x <= x_end; x++) {
		for (int y = 0; y < number; y++) {
			for (int z = 0; z < number; z++) {
				if (cubic[x * number * number + y * number + z].state != PLAGUE) {
					cubic[x * number * number + y * number + z].state = cubic[x * number * number + y * number + z].next_prev_state;
					cubic[x * number * number + y * number + z].next_prev_state = NONE;
				}
			}
		}
	}

	return NULL;
}


cell* get_cell(int x, int y, int z) {

	if ((x >= 0) && (y >= 0) && (z >= 0))
		if ((x < number) && (y < number) && (z < number))
			return &cubic[x * number * number + y * number + z];

	return NULL;
}


int get_devil_count_in_submatrix(int min_x, int max_x, int min_y, int max_y, int min_z, int max_z) {
	int devil_count = 0;

	for (int x = min_x; x <= max_x; x++) {
		for (int y = min_y; y <= max_y; y++) {
			for (int z = min_z; z <= max_z; z++) {
					devil_count += devil_cubic[x * number * number + y * number + z].on_devil;
			}
		}
	}

	return devil_count;
}

vec3 set_angel_direction(int* devil_count_arr) {
	vec3 direction;
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
		printf("Angel Direction ERROR.\n");
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


int move_angel(int* angel_direction) {
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

void purify_devil_plague(int angel_scope) {
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
				if (devil_cubic[x * number * number + y * number + z].on_devil > 0)
					devil_cubic[x * number * number + y * number + z].on_devil = 0;
				if (cubic[x * number * number + y * number + z].state == PLAGUE) {
					cubic[x * number * number + y * number + z].state = cubic[x * number * number + y * number + z].next_prev_state;
					cubic[x * number * number + y * number + z].next_prev_state = NONE;
				}
			}
		}
	}
}




//////////////////////////////////////////////////////////////

void init_resources(struct setup *s) {
	_s = s;
	number = _s->map_size;

	make_cubic(s);
	init_cubic(s);
	init_angel(s);
	init_devil(s);

	init_thread(s);
}

void devil_stage(struct setup *s) {
	//if devil count is 0, create a new devil
	if (get_devil_count() == 0)
		deploy_devil(1);

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

	devil_move->x = move[0];
	devil_move->y = move[1];
	devil_move->z = move[2];


	//move all devils.
	//USING PTHREAD
	move_all_devils_and_spread_plague();

    //set thread counter to zero.
    init_thread_counter();

	//copy devil.
	int devil_count = get_devil_count();
	deploy_devil(devil_count);

}

void live_dead_stage(struct setup *s) {
	//Update both Live & Dead stage and Plague stage at the same time.

	for (int i = 0; i < thread_num; i++) {
		pthread_create(&thread_range_list[i].id, NULL, PT_update_cell_state, (void*)&thread_range_list[i].i);
	}

	for (int i = 0; i < thread_num; i++) {
		pthread_join(thread_range_list[i].id, NULL);
	}

	init_thread_counter();
}

void plague_stage (struct setup *s) {
	//do nothing. it is already done in live & dead stage.
}

void angel_stage (struct setup *s) {
	//set direction, move value
	int devil_count[6];
	devil_count[PLUS_X] = get_devil_count_in_submatrix(cubic_angel->x + 1, number - 1, 0, number - 1, 0, number - 1);
	devil_count[PLUS_Y] = get_devil_count_in_submatrix(0, number - 1, cubic_angel->y + 1, number - 1, 0, number - 1);
	devil_count[PLUS_Z] = get_devil_count_in_submatrix(0, number - 1, 0, number - 1, cubic_angel->z + 1, number - 1);
	devil_count[MINUS_X] = get_devil_count_in_submatrix(0, cubic_angel->x - 1, 0, number - 1, 0, number - 1);
	devil_count[MINUS_Y] = get_devil_count_in_submatrix(0, number - 1, 0, cubic_angel->y - 1, 0, number - 1);
	devil_count[MINUS_Z] = get_devil_count_in_submatrix(0, number - 1, 0, number - 1, 0, cubic_angel->z - 1);

	vec3 angel_direction = set_angel_direction(devil_count);

	int move_value = (int)(number / 10);

	int angel_move[3] = { 0,0,0 };
	angel_move[0] = angel_direction.x * move_value;
	angel_move[1] = angel_direction.y * move_value;
	angel_move[2] = angel_direction.z * move_value;

	//move and return real move value
	
	move_value = move_angel(angel_move);

	
	//set scope
	int angel_scope = (int)(get_devil_count() / (5 * number * number));
	int X = (int)(move_value / 2);

	if (X > angel_scope)
		angel_scope = X;


	//purify
	purify_devil_plague(angel_scope);


}



void print_init_map (struct setup *s) {
	FILE *f;
	f = fopen("Initial_map.txt", "w");
	if (f == NULL) {
		printf("File Load Error\n");
		exit(1);
	}


	for (int x = 0; x < number; x++) {
		for (int y = 0; y < number; y++) {
			for (int z = 0; z < number; z++) {
				if (cubic[x * number * number + y * number + z].state == DEAD)
					fprintf(f, "D ");
				else if (cubic[x * number * number + y * number + z].state == LIVE)
					fprintf(f, "L ");
				else if (cubic[x * number * number + y * number + z].state == PLAGUE)
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
		printf("File Load Error\n");
		exit(1);
	}
	fprintf(f, "[Angel]\n");
	fprintf(f, "(%d, %d, %d)\n\n", cubic_angel->x, cubic_angel->y, cubic_angel->z);

	int x, y, z;
	for (int i = 0; i < number; i++) {
		for (int j = 0; j < number; j++) {
			for (int k = 0; k < number; k++) {
				if (devil_cubic[i * number * number + j * number + k].on_devil > 0) {
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
		printf("File Load Error\n");
		exit(1);
	}


	for (int x = 0; x < number; x++) {
		for (int y = 0; y < number; y++) {
			for (int z = 0; z < number; z++) {
				if (cubic[x * number * number + y * number + z].state == DEAD)
					fprintf(f, "D ");
				else if (cubic[x * number * number + y * number + z].state == LIVE)
					fprintf(f, "L ");
				else if (cubic[x * number * number + y * number + z].state == PLAGUE)
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
		printf("File Load Error\n");
		exit(1);
	}
	fprintf(f, "[Angel]\n");
	fprintf(f, "(%d, %d, %d)\n\n", cubic_angel->x, cubic_angel->y, cubic_angel->z);


	fprintf(f, "[Devil]\n");

	for (int x = 0; x < number; x++) {
		for (int y = 0; y < number; y++) {
			for (int z = 0; z < number; z++) {
				while (devil_cubic[x * number * number + y * number + z].on_devil > 0) {
					fprintf(f, "(%d, %d, %d)\n", x, y, z);
					devil_cubic[x * number * number + y * number + z].on_devil--;
				}
			}
		}
	}

	fclose(f);
	
}

void free_resources (struct setup *s) {
	free(cubic);
	free(devil_cubic);
	free(cubic_angel);
	free(thread_range_list);
	free(devil_move);
}

void run_game(struct setup* s) {
	int i = 0;
	while (i++ < s->total_loop) {
		devil_stage(s);

		live_dead_stage(s);

		plague_stage(s);

		angel_stage(s);

	}
}
