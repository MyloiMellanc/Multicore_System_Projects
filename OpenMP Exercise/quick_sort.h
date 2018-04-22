#ifndef QUICK_SORT_MP
#define QUICK_SORT_MP

#include <omp.h>

#define QUEUE_SIZE 50


typedef struct Work
{
    int q;
    int r;
}work;



work work_queue[QUEUE_SIZE];

int queue_start;
int queue_end;


void init_queue();
int is_work_on_queue();
int push_work_queue(int q, int r);
int get_work(work* _work);

void sort(unsigned char arr[], int q, int r);
void quicksort(unsigned char arr[], int q, int r);



#endif
