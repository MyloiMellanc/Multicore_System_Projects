quicksort: main.c quick_sort.h quick_sort.c lcgrand.o
	gcc -fopenmp -o quicksort main.c quick_sort.h quick_sort.c lcgrand.o
lcgrand.o: lcgrand.c lcgrand.h
	gcc -c lcgrand.c lcgrand.h

clean:
	rm *.o
	rm *.gch
	rm quicksort
