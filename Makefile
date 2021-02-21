main: main.o my_malloc.o
	cc main.o my_malloc.o -o main

main.o: main.c
	cc -c main.c -o main.o

my_malloc.o: my_malloc.c
	cc -c my_malloc.c -o my_malloc.o

clean:
	rm -f main.o my_malloc.o
