cannon_nonblk: main.c cannon.h cannon_nonblk.c lcgrand.o
	mpicc -o cannon_nonblk main.c cannon.h cannon_nonblk.c lcgrand.o
cannon_blk: main.c cannon.h cannon_blk.c lcgrand.o
	mpicc -o cannon_blk main.c cannon.h cannon_blk.c lcgrand.o
lcgrand.o: lcgrand.c lcgrand.h
	gcc -c lcgrand.c lcgrand.h

clean:
	rm *.o
	rm *.gch
	rm cannon_nonblk
	rm cannon_blk
