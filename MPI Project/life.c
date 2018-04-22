#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "./setup.h"
#include "./game.h"

static void initialize (void);
static void end (void);
static void parameter_setup ();
static double mySecond (void);

int main (void) {
	double total_time;

	/* System setup start */
	initialize();
	/* System setup end */

	total_time = mySecond();

	run_game();
	//body

	total_time = mySecond() - total_time;

	if(comm_rank == 0)
		fprintf(stderr, "%lf\n", total_time);

	sleep(5);

	end();
    
    return 0;
}

static void initialize (void) {
	parameter_setup();	// Parameter setting
	init_resources();
	print_init_map();
	print_init_pos();
}


static void end (void) {
	print_fin_map();
	print_fin_pos();
	free_resources();
}

static void parameter_setup () {
	FILE *rfp = fopen("./input.life", "r");
	char trash[32];

	/* Parameters about Game of Life */
	fscanf(rfp, "%s %d", trash, &s.core_num);
	fscanf(rfp, "%s %d", trash, &s.total_loop);
	fscanf(rfp, "%s %d", trash, &s.map_size);
	fscanf(rfp, "%s %d", trash, &s.dead_min);
	fscanf(rfp, "%s %d", trash, &s.dead_max);
	fscanf(rfp, "%s %d", trash, &s.live_min);
	fscanf(rfp, "%s %d", trash, &s.live_max);
	/* Random seeds for map */
	fscanf(rfp, "%s %d", trash, &s.SEED_MAP);
	/* Random seeds for devil */
	fscanf(rfp, "%s %d", trash, &s.SEED_DVL_GEN_X);
	fscanf(rfp, "%s %d", trash, &s.SEED_DVL_GEN_Y);
	fscanf(rfp, "%s %d", trash, &s.SEED_DVL_GEN_Z);
	fscanf(rfp, "%s %d", trash, &s.SEED_DVL_MOV_X);
	fscanf(rfp, "%s %d", trash, &s.SEED_DVL_MOV_Y);
	fscanf(rfp, "%s %d", trash, &s.SEED_DVL_MOV_Z);
	
	fclose(rfp);
}

static double mySecond (void) {
	struct timeval tp;
	struct timezone tzp;
	int i;

	i = gettimeofday(&tp, &tzp);

	return ((double)tp.tv_sec + (double)tp.tv_usec * 1.e-6);
}
