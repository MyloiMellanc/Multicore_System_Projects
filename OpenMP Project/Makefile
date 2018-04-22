life: life.c lcgrand.o game.o
	gcc -fopenmp -o life life.c lcgrand.o game.o
lcgrand.o: lcgrand.c lcgrand.h
	gcc -c lcgrand.c lcgrand.h 
game.o: game.c game.h
	gcc -fopenmp -c game.c game.h

clean:
	rm *.o
	rm *.gch
