clever: list.o main.o misc.o scan.o clever.o files.o
	-cd ../diss; if test ! -r diss.o; then make diss.o; fi
	cc -o clever ../diss/diss.o main.o list.o misc.o scan.o clever.o files.o

main.o: list.h
misc.o: main.h ../diss/decode.h
list.o: list.h main.h
scan.o: list.h main.h ../diss/decode.h
clever.o: list.h
files.o: ../ass.out.h

lint:
	lint $(CFLAGS) *.c ../diss/*.c > lint.out
