life: life.c lcgrand.o game.o
	mpicc -o life life.c lcgrand.o game.o -lm
lcgrand.o: lcgrand.c lcgrand.h
	gcc -c lcgrand.c lcgrand.h 
game.o: game.c game.h
	mpicc -std=c99 -c game.c game.h -lm

clean:
	rm *.o
	rm *.gch
	rm life
