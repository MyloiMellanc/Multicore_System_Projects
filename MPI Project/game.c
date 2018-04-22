
#include "game.h"


void init_thread()
{
    MPI_Init(NULL,NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    
    
    float diff = (float)s.map_size / (float)s.core_num;
    diff = (roundf(diff * 10.0) / 10.0) + 0.05;
    int x1, x2;
    
    if(comm_rank != comm_size - 1) {
        x1 = (int)(comm_rank * diff);
        x2 = (int)(comm_rank * diff + diff - 1.0);
        x_position = x1;
        x_range = (int)(x2 - x1 + 1);
    }
    else {
        x1 = (int)(comm_rank * diff);
        x2 = s.map_size - 1;
        x_position = x1;
        x_range = (int)(x2 - x1 + 1);
    }
    
    ////set cubic type
    const int nitems = 2;
    int block_length[2] = {1,1};
    MPI_Datatype types[2] = {MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR};
    MPI_Aint offsets[2];
    
    offsets[0] = offsetof(cell,state);
    offsets[1] = offsetof(cell,next_prev_state);
    
    MPI_Type_create_struct(nitems,block_length, offsets, types, &cubic_type);
    MPI_Type_commit(&cubic_type);
}

void make_cubic()
{
    int x_num;
    
    if(comm_rank == 0) {
        x_min = 0;
        x_start = 0;
        x_end = x_range - 1;
        x_max = x_range;
        
        x_num = x_range + 1;
    }
    else if((0 < comm_rank) && (comm_rank < comm_size - 1)) {
        x_min = 0;
        x_start = 1;
        x_end = x_range;
        x_max = x_range + 1;
        
        x_num = x_range + 2;
    }
    else if(comm_rank == comm_size - 1) {
        x_min = 0;
        x_start = 1;
        x_end = x_range;
        x_max = x_range;
        
        x_num = x_range + 1;
    }
    
    cubic = (cell*)malloc(sizeof(cell) * x_num * map_size * map_size);
    devil_cubic = (short*)malloc(sizeof(short) * x_num * map_size * map_size);  
}

void init_cubic()
{
    int stream;
    long zset;
    
    if(comm_rank != 0) {
        MPI_Recv(&stream, 1, MPI_INT, comm_rank - 1, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&zset, 1, MPI_LONG, comm_rank - 1, 0, MPI_COMM_WORLD, &status);
        lcgrandst(zset, stream);
    }
    else {
        stream = s.SEED_MAP;
    }
    
    char lived;
    for(int x = x_start; x <= x_end; x++) {
        for(int y = 0; y < map_size; y++) {
            for(int z = 0; z < map_size; z++) {
                lived = uniform(0, 1, stream);
                cubic[x * map_size * map_size + y * map_size + z].state = lived;
                cubic[x * map_size * map_size + y * map_size + z].next_prev_state = NONE;
            }
        }
    }
    
    if(comm_rank != comm_size - 1) {
        zset = lcgrandgt(stream);
        MPI_Isend(&stream, 1, MPI_INT, comm_rank + 1, 0, MPI_COMM_WORLD, &request_1);
        MPI_Isend(&zset, 1, MPI_LONG, comm_rank + 1, 0, MPI_COMM_WORLD, &request_2);
    }
    
    for(int x = x_min; x <= x_max; x++) {
        for(int y = 0; y < map_size; y++) {
            for(int z = 0; z < map_size; z++) {
                devil_cubic[x * map_size * map_size + y * map_size + z] = 0;
            }
        }
    }

    if(comm_rank != comm_size - 1) {
        MPI_Wait(&request_1, &status);
        MPI_Wait(&request_2, &status);
    }
    
}

void init_angel()
{
    double angel_pos_double = (double)s.map_size / 2.0f;
    int angel_pos = s.map_size / 2;
    
    if (angel_pos_double - (double)angel_pos >= 0.5)
        angel_pos++;
    
    angel_pos--;
    
    angel.x = angel_pos;
    angel.y = angel_pos;
    angel.z = angel_pos;
    
    init_angel_sight();
}

void init_devil()
{
    int devil_x = uniform(0, map_size - 1, s.SEED_DVL_GEN_X);
    int devil_y = uniform(0, map_size - 1, s.SEED_DVL_GEN_Y);
    int devil_z = uniform(0, map_size - 1, s.SEED_DVL_GEN_Z);
    
    if((x_position <= devil_x) && (devil_x < x_position + x_range)) {
        int temp_devil_x = devil_x - x_position + x_start;
        devil_cubic[temp_devil_x * map_size * map_size + devil_y * map_size + devil_z] = 1;
        set_cell_plagued(temp_devil_x, devil_y, devil_z);
    }
    
    devil_count = 1;
}


void init_angel_sight()
{
    for(int i = 0; i < 6; i++)
        angel_sight[i] = 0;
}


void set_cell_plagued(int x, int y, int z) {
    if (cubic[x * map_size * map_size + y * map_size + z].state != PLAGUE) {
        cubic[x * map_size * map_size + y * map_size + z].next_prev_state = cubic[x * map_size * map_size + y * map_size + z].state;
        cubic[x * map_size * map_size + y * map_size + z].state = PLAGUE;
    }
}

void check_angel_sight(int x, int y, int z, int devil_num)
{
    if(angel.x < x)
        angel_sight[PLUS_X] += devil_num;
    else if(angel.x > x)
        angel_sight[MINUS_X] += devil_num;
    
    if(angel.y < y)
        angel_sight[PLUS_Y] += devil_num;
    else if(angel.y > y)
        angel_sight[MINUS_Y] += devil_num;
    
    if(angel.z < z)
        angel_sight[PLUS_Z] += devil_num;
    else if(angel.z > z)
        angel_sight[MINUS_Z] += devil_num;
}

void move_devils()
{
    int temp_x_start = x_start; int temp_y_start = 0;       int temp_z_start = 0;
    int temp_x_end = x_end + 1; int temp_y_end = map_size;  int temp_z_end = map_size;
    int add_x = 1;              int add_y = 1;              int add_z = 1;
    
    if(devil_move.x == 1) {
        temp_x_start = x_end;
        temp_x_end = x_start - 1;
        add_x = -1;
    }
    if(devil_move.y == 1) {
        temp_y_start = map_size - 1;
        temp_y_end = -1;
        add_y = -1;
    }
    if(devil_move.z == 1) {
        temp_z_start = map_size - 1;
        temp_z_end = -1;
        add_z = -1;
    }
    
    if(x_min != x_start)
        for(int y = 0; y < map_size; y++) {
            for(int z = 0; z < map_size; z++) {
                devil_cubic[x_min * map_size * map_size + y * map_size + z] = 0;
            }
        }
    if(x_max != x_end)
        for(int y = 0; y < map_size; y++) {
            for(int z = 0; z < map_size; z++) {
                devil_cubic[x_max * map_size * map_size + y * map_size + z] = 0;
            }
        }
    
    int temp_move_x, temp_move_y, temp_move_z;
    for(int x = temp_x_start; x != temp_x_end; x += add_x) {
        for(int y = temp_y_start; y != temp_y_end; y += add_y) {
            for(int z = temp_z_start; z != temp_z_end; z+= add_z) {
                if(devil_cubic[x * map_size * map_size + y * map_size + z] > 0) {
                    if ((x + devil_move.x >= x_min) && (x + devil_move.x <= x_max))
                        temp_move_x = devil_move.x;
                    else
                        temp_move_x = 0;
                    
                    if ((y + devil_move.y >= 0) && (y + devil_move.y < map_size))
                        temp_move_y = devil_move.y;
                    else
                        temp_move_y = 0;
                    
                    if ((z + devil_move.z >= 0) && (z + devil_move.z < map_size))
                        temp_move_z = devil_move.z;
                    else
                        temp_move_z = 0;
                    
                    
                    temp_move_x = x + temp_move_x;
                    temp_move_y = y + temp_move_y;
                    temp_move_z = z + temp_move_z;
                    
                    devil_cubic[x * map_size * map_size + y * map_size + z] = 0;
                    devil_cubic[temp_move_x * map_size * map_size + temp_move_y * map_size + temp_move_z] = 1;
                }
            }
        }
    }
    
    if(devil_move.x == 1) {
        //send x_max (except comm_size - 1)
        if(comm_rank != comm_size - 1)
            MPI_Send(&devil_cubic[x_max * map_size * map_size], map_size * map_size, MPI_SHORT, comm_rank + 1, 0, MPI_COMM_WORLD);
        
        //recv x_start (except 0)
        if(comm_rank != 0)
            MPI_Recv(&devil_cubic[x_start * map_size * map_size], map_size * map_size, MPI_SHORT, comm_rank - 1, 0, MPI_COMM_WORLD, &status);

    }
    else if(devil_move.x == -1) {
        //send x_min (except 0)
        if(comm_rank != 0)
            MPI_Send(&devil_cubic[x_min * map_size * map_size], map_size * map_size, MPI_SHORT, comm_rank - 1, 0, MPI_COMM_WORLD);
        
        //recv x_end (except comm_size - 1)
        if(comm_rank != comm_size - 1)
            MPI_Recv(&devil_cubic[x_end * map_size * map_size], map_size * map_size, MPI_SHORT, comm_rank + 1, 0, MPI_COMM_WORLD, &status);
    }
}

void spread_plague()
{
    for(int x = x_start; x <= x_end; x++) {
        for(int y = 0; y < map_size; y++) {
            for(int z = 0; z < map_size; z++) {
                if(devil_cubic[x * map_size * map_size + y * map_size + z] > 0) {
                    devil_count += devil_cubic[x * map_size * map_size + y * map_size + z];
                    set_cell_plagued(x, y, z);
                    
                    check_angel_sight(x - x_start + x_position, y, z, devil_cubic[x * map_size * map_size + y * map_size + z]);
                }
            }
        }
    }
}


cell* get_cell(int x, int y, int z) {
    if ((x >= x_min) && (y >= 0) && (z >= 0))
        if ((x <= x_max) && (y < map_size) && (z < map_size))
            return &cubic[x * map_size * map_size + y * map_size + z];
    
    return NULL;
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

void update_cell_toggle_and_plague(char* cube_stack, char* stack_pointer, char* cube_state, int x, int y, int z) {
    unsigned char current_state = get_cell(x, y, z)->state;
    
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
        if ((live_num > s.live_min) && (live_num  < s.live_max))
            current_state = LIVE;
        else
            current_state = DEAD;
    }
    else if (current_state == LIVE) {
        if ((live_num < s.dead_min) || (live_num > s.dead_max))
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
    //cubic[x * map_size * map_size + y * map_size + z].next_prev_state = current_state;
}

vec3 set_angel_direction(unsigned long* devil_count_arr) {
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
    int before[3] = { angel.x, angel.y, angel.z };
    
    angel.x = angel.x + angel_direction[0];
    if (angel.x < 0)
        angel.x = 0;
    else if (angel.x >= map_size)
        angel.x = map_size - 1;
    
    angel.y = angel.y + angel_direction[1];
    if (angel.y < 0)
        angel.y = 0;
    else if (angel.y >= map_size)
        angel.y = map_size - 1;
    
    angel.z = angel.z + angel_direction[2];
    if (angel.z < 0)
        angel.z = 0;
    else if (angel.z >= map_size)
        angel.z = map_size - 1;
    
    int after[3] = { angel.x, angel.y, angel.z };
    
    if (after[0] - before[0] != 0)
        return abs(after[0] - before[0]);
    else if (after[1] - before[1] != 0)
        return abs(after[1] - before[1]);
    else if (after[2] - before[2] != 0)
        return abs(after[2] - before[2]);
    
    return map_size / 10;
    
    
}

void purify_devil_plague(int angel_scope) {
    
    
    int min_x = angel.x - angel_scope;
    int max_x = angel.x + angel_scope;
    
    if (min_x < x_position)
        min_x = x_start;
    else if(min_x >= x_position + x_range)
        return;
    else
        min_x = min_x - x_position + x_start;
    
    if (max_x >= x_position + x_range)
        max_x = x_end;
    else if(max_x < x_position)
        return;
    else
        max_x = max_x - x_position + x_start;
    
    
    int min_y = angel.y - angel_scope;
    if (min_y < 0)
        min_y = 0;
    int max_y = angel.y + angel_scope;
    if (max_y >= map_size)
        max_y = map_size - 1;
    
    int min_z = angel.z - angel_scope;
    if (min_z < 0)
        min_z = 0;
    int max_z = angel.z + angel_scope;
    if (max_z >= map_size)
        max_z = map_size - 1;
    
    for (int x = min_x; x <= max_x; x++) {
        for (int y = min_y; y <= max_y; y++) {
            for (int z = min_z; z <= max_z; z++) {
                if (devil_cubic[x * map_size * map_size + y * map_size + z] > 0) {
                    corpse += devil_cubic[x * map_size * map_size + y * map_size + z];
                    devil_cubic[x * map_size * map_size + y * map_size + z] = 0;
                }
                if (cubic[x * map_size * map_size + y * map_size + z].state == PLAGUE) {
                    cubic[x * map_size * map_size + y * map_size + z].state = cubic[x * map_size * map_size + y * map_size + z].next_prev_state;
                    cubic[x * map_size * map_size + y * map_size + z].next_prev_state = NONE;
                }
            }
        }
    }
}



void init_resources () {
    map_size = s.map_size;
    
    init_thread();
    toggle = 1;
    
    make_cubic();
    init_cubic();
    init_angel();
    init_devil();
    
    MPI_Barrier(MPI_COMM_WORLD);
}

void run_game () {
	int i = 0;
    
	while(i++ < s.total_loop) {
		devil_stage();
		MPI_Barrier(MPI_COMM_WORLD);
		live_dead_plague_stage();
		MPI_Barrier(MPI_COMM_WORLD);
        angel_stage();
        MPI_Barrier(MPI_COMM_WORLD);
	}
    
}

void devil_stage () {
    if(devil_count == 0) {
        int devil_x = uniform(0, map_size - 1, s.SEED_DVL_GEN_X);
        int devil_y = uniform(0, map_size - 1, s.SEED_DVL_GEN_Y);
        int devil_z = uniform(0, map_size - 1, s.SEED_DVL_GEN_Z);
        
        if((x_position <= devil_x) && (devil_x < x_position + x_range)) {
            int temp_devil_x = devil_x - x_position + x_start;
            devil_cubic[temp_devil_x * map_size * map_size + devil_y * map_size + devil_z] += 1;
            devil_count++;
        }
    }
    
    int move[3];
    move[0] = uniform(0, 2, s.SEED_DVL_MOV_X);
    move[1] = uniform(0, 2, s.SEED_DVL_MOV_Y);
    move[2] = uniform(0, 2, s.SEED_DVL_MOV_Z);
    
    for (int i = 0; i < 3; i++) {
        if (move[i] == 0)
            move[i] = 1;
        else if (move[i] == 1)
            move[i] = 0;
        else if (move[i] == 2)
            move[i] = -1;
    }
    
    devil_move.x = move[0];
    devil_move.y = move[1];
    devil_move.z = move[2];

    move_devils();

    devil_count = 0;
    init_angel_sight();
    
    spread_plague();
    
    unsigned long devil_count_temp;
    
    MPI_Allreduce(&devil_count, &devil_count_temp, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
    

    for(int i = 0; i < devil_count_temp; i++) {
        int devil_x = uniform(0, map_size - 1, s.SEED_DVL_GEN_X);
        int devil_y = uniform(0, map_size - 1, s.SEED_DVL_GEN_Y);
        int devil_z = uniform(0, map_size - 1, s.SEED_DVL_GEN_Z);
    
        if((x_position <= devil_x) && (devil_x < x_position + x_range)) {
            int temp_devil_x = devil_x - x_position + x_start;
            devil_cubic[temp_devil_x * map_size * map_size + devil_y * map_size + devil_z]++;
            check_angel_sight(devil_x, devil_y, devil_z, 1);
            devil_count++;
        }
    }
}

void live_dead_plague_stage () {
    ///send start, end to min, max
    if(comm_rank != comm_size - 1)
        MPI_Send(&cubic[x_end * map_size * map_size], map_size * map_size, cubic_type, comm_rank + 1, 0, MPI_COMM_WORLD);
    
    if(comm_rank != 0) {
        MPI_Recv(&cubic[x_min * map_size * map_size], map_size * map_size, cubic_type, comm_rank - 1, 0, MPI_COMM_WORLD, &status);
        MPI_Send(&cubic[x_start * map_size * map_size], map_size * map_size, cubic_type, comm_rank - 1, 0, MPI_COMM_WORLD);
    }
    
    if(comm_rank != comm_size - 1)
        MPI_Recv(&cubic[x_max * map_size * map_size], map_size * map_size, cubic_type, comm_rank + 1, 0, MPI_COMM_WORLD, &status);
    
    //Don't Control Cell which state is PLAGUE.
    
    char cube_stack[27];
    char stack_pointer;
    char cube_state[3];
    
    stack_initialize(cube_stack, &stack_pointer, cube_state);
    
    for (int x = x_start; x <= x_end; x++) {
        for (int y = 0; y < map_size; y++) {
            for (int z = 0; z < map_size; z++) {
                update_cell_toggle_and_plague(cube_stack, &stack_pointer, cube_state, x, y, z);
            }
            
            stack_initialize(cube_stack, &stack_pointer, cube_state);
        }
    }
    
    for (int x = x_start; x <= x_end; x++) {
        for (int y = 0; y < map_size; y++) {
            for (int z = 0; z < map_size; z++) {
                if (cubic[x * map_size * map_size + y * map_size + z].state != PLAGUE) {
                    cubic[x * map_size * map_size + y * map_size + z].state = cubic[x * map_size * map_size + y * map_size + z].next_prev_state;
                    cubic[x * map_size * map_size + y * map_size + z].next_prev_state = NONE;
                }
            }
        }
    }
}

void angel_stage () {
    MPI_Allreduce(MPI_IN_PLACE, angel_sight, 6, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
    
    vec3 angel_direction = set_angel_direction(angel_sight);
    
    int move_value = (int)(map_size / 10);
    
    int angel_move[3] = { 0,0,0 };
    angel_move[0] = angel_direction.x * move_value;
    angel_move[1] = angel_direction.y * move_value;
    angel_move[2] = angel_direction.z * move_value;
    
    //move and return real move value
    
    move_value = move_angel(angel_move);
    
    MPI_Allreduce(MPI_IN_PLACE, &devil_count, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
    

    //set scope
    int angel_scope = (int)(devil_count / (5 * map_size * map_size));
    int X = (int)(move_value / 2);
    
    if (X > angel_scope)
        angel_scope = X;
    
    corpse = 0;
    //purify
    purify_devil_plague(angel_scope);

    MPI_Allreduce(MPI_IN_PLACE, &corpse, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
    devil_count -= corpse;
    
}

void print_init_map () {
    FILE *f;
    
    if(comm_rank != 0)
        MPI_Recv(&toggle, 1, MPI_INT, comm_rank - 1, 0, MPI_COMM_WORLD, &status);

    if(comm_rank == 0)
        f = fopen("Initial_map.txt", "w");
    else
        f = fopen("Initial_map.txt", "a");
    
    if (f == NULL) {
        printf("File Load Error\n");
        exit(1);
    }
    
    for (int x = x_start; x <= x_end; x++) {
        for (int y = 0; y < map_size; y++) {
            for (int z = 0; z < map_size; z++) {
                if (cubic[x * map_size * map_size + y * map_size + z].state == DEAD)
                    fprintf(f, "D ");
                else if (cubic[x * map_size * map_size + y * map_size + z].state == LIVE)
                    fprintf(f, "L ");
                else if (cubic[x * map_size * map_size + y * map_size + z].state == PLAGUE)
                    fprintf(f, "P ");
            }
            fprintf(f, "\n");
        }
        fprintf(f, "\n");
    }
    
    fclose(f);

    if(comm_rank != comm_size - 1)
        MPI_Send(&toggle, 1, MPI_INT, comm_rank + 1, 0, MPI_COMM_WORLD);

}

void print_init_pos () {
    FILE* f;
    
    if(comm_rank != 0)
        MPI_Recv(&toggle, 1, MPI_INT, comm_rank - 1, 0, MPI_COMM_WORLD, &status);

    if(comm_rank == 0)
        f = fopen("Initial_pos.txt", "w");
    else
        f = fopen("Initial_pos.txt", "a");
    if (f == NULL) {
        printf("File Load Error\n");
        exit(1);
    }
    
    if(comm_rank == 0) {
        fprintf(f, "[Angel]\n");
        fprintf(f, "(%d, %d, %d)\n\n", angel.x, angel.y, angel.z);
        
        fprintf(f, "[Devil]\n");
    }
    
    int x, y, z;
    for (int i = x_start; i <= x_end; i++) {
        for (int j = 0; j < map_size; j++) {
            for (int k = 0; k < map_size; k++) {
                if (devil_cubic[i * map_size * map_size + j * map_size + k] > 0) {
                    x = i + x_position - x_start;
                    y = j;
                    z = k;
                    
                    fprintf(f, "(%d, %d, %d)\n", x, y, z);
                    
                    break;
                }
            }
        }
    }
    
    fclose(f);

    if(comm_rank != comm_size - 1)
        MPI_Send(&toggle, 1, MPI_INT, comm_rank + 1, 0, MPI_COMM_WORLD);

}

void print_fin_map () {
    FILE *f;
    
    if(comm_rank != 0)
        MPI_Recv(&toggle, 1, MPI_INT, comm_rank - 1, 0, MPI_COMM_WORLD, &status);

    if(comm_rank == 0)
        f = fopen("Final_map.txt", "w");
    else
        f = fopen("Final_map.txt", "a");
    
    if (f == NULL) {
        printf("File Load Error\n");
        exit(1);
    }
    
    for (int x = x_start; x <= x_end; x++) {
        for (int y = 0; y < map_size; y++) {
            for (int z = 0; z < map_size; z++) {
                if (cubic[x * map_size * map_size + y * map_size + z].state == DEAD)
                    fprintf(f, "D ");
                else if (cubic[x * map_size * map_size + y * map_size + z].state == LIVE)
                    fprintf(f, "L ");
                else if (cubic[x * map_size * map_size + y * map_size + z].state == PLAGUE)
                    fprintf(f, "P ");
            }
            fprintf(f, "\n");
        }
        fprintf(f, "\n");
    }
    
    fclose(f);

    if(comm_rank != comm_size - 1)
        MPI_Send(&toggle, 1, MPI_INT, comm_rank + 1, 0, MPI_COMM_WORLD);
}

void print_fin_pos () {
    FILE* f;
    
    if(comm_rank != 0)
        MPI_Recv(&toggle, 1, MPI_INT, comm_rank - 1, 0, MPI_COMM_WORLD, &status);

    if(comm_rank == 0)
        f = fopen("Final_pos.txt", "w");
    else
        f = fopen("Final_pos.txt", "a");
    if (f == NULL) {
        printf("File Load Error\n");
        exit(1);
    }
    
    if(comm_rank == 0) {
        fprintf(f, "[Angel]\n");
        fprintf(f, "(%d, %d, %d)\n\n", angel.x, angel.y, angel.z);
        
        fprintf(f, "[Devil]\n");
    }
    
    int x, y, z;
    for (int i = x_start; i <= x_end; i++) {
        for (int j = 0; j < map_size; j++) {
            for (int k = 0; k < map_size; k++) {
                while (devil_cubic[i * map_size * map_size + j * map_size + k] > 0) {
                    x = i + x_position - x_start;
                    y = j;
                    z = k;
                
                    fprintf(f, "(%d, %d, %d)\n", x, y, z);
                    devil_cubic[i * map_size * map_size + j * map_size + k]--;
                }
            }
        }
    }

    fclose(f);
    
    if(comm_rank != comm_size - 1)
        MPI_Send(&toggle, 1, MPI_INT, comm_rank + 1, 0, MPI_COMM_WORLD);
}

void free_resources () {
    free(cubic);
    free(devil_cubic);
    MPI_Type_free(&cubic_type);
    MPI_Finalize();
}




