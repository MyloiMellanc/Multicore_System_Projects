

#include "quick_sort.h"



void init_queue(){
    queue_start = 0;
    queue_end = 0;
}


int is_work_on_queue(){
    if(queue_start != queue_end)
        return 1;
    else
        return 0;
}


int is_full_queue(){
    if(((queue_end + 1) % QUEUE_SIZE) == queue_start)
        return 1;
    else
        return 0;
}

int push_work_queue(int q, int r){
    int state = 0;

	#pragma omp critical(work)
    if(is_full_queue() == 0) {
        queue_end = (queue_end + 1) % QUEUE_SIZE;
        work_queue[queue_end].q = q;
        work_queue[queue_end].r = r;
                
        state = 1;
    }
    
    return state;
}


int get_work(work* _work) {
    int state = 0;
    
	#pragma omp critical(work)
        if(is_work_on_queue() == 1)
        {
            queue_start = (queue_start + 1) % QUEUE_SIZE;
            _work->q = work_queue[queue_start].q;
            _work->r = work_queue[queue_start].r;
            state = 1;
            
        }
    
    return state;
}


void sort(unsigned char arr[], int q, int r) {
	int pivot;
	int index;
	int temp;
	int i;
    
    
	if(q < r)
    {
		pivot = arr[q];
		index = q;
        
		for(i = q + 1; i <= r; i++) {
			if(arr[i] < pivot) {
				index += 1;

				temp = arr[index];
				arr[index] = arr[i];
				arr[i] = temp;
			}
		}
        
		temp = arr[q];
		arr[q] = arr[index];
		arr[index] = temp;
        

        if(index - r < 5000){
            sort(arr, q, index);
        } else {
            if(push_work_queue(q, index) == 0)
                sort(arr, q, index);
        }
        
        if(r - index - 1 < 5000){
            sort(arr, index+1, r);
        } else {
            if(push_work_queue(index+1, r) == 0)
                sort(arr, index+1, r);
        }
	}
}


void quicksort(unsigned char arr[], int q, int r) {
    init_queue();
    
    sort(arr, q, r);
    
    int working = 0;
    
    work _work;
    
    #pragma omp parallel num_threads(12) shared(working, arr) private(_work)
    {
        while((working > 0) || (is_work_on_queue() == 1)) {
            if(get_work(&_work) == 1){
                #pragma omp atomic
                working++;
                
                sort(arr, _work.q, _work.r);
        
                #pragma omp atomic
                working--;
            }
        }
    }
}




