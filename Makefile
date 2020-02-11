all : dshell clean

dshell : main.o shell.o procesos.o hilos.o listas.o 
	gcc -Wall -g -o dshell main.o shell.o procesos.o hilos.o listas.o -lpthread -lrt 
	
main.o : main.c shell.h procesos.h hilos.h
	gcc -c -g -Wall main.c
	
shell.o : shell.c shell.h listas.h
	gcc -c -g -Wall shell.c

procesos.o : procesos.c procesos.h shell.h
	gcc -c -g -Wall procesos.c
	
hilos.o : hilos.c hilos.h shell.h procesos.c
	gcc -c -g -Wall hilos.c

listas.o : listas.c listas.h
	gcc -c -g -Wall listas.c
	
.PHONY: clean
clean:
	rm main.o shell.o procesos.o hilos.o listas.o
