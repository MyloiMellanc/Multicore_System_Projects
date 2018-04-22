life: life.c lcgrand.o game.o
	gcc -o life life.c lcgrand.o game.o -lpthread
lcgrand.o: lcgrand.c lcgrand.h
	gcc -c lcgrand.c lcgrand.h 
game.o: game.c game.h
	gcc -c game.c game.h -lpthread

clean:
	rm *.o
	rm *.gch
