#SRCS = libmem1.so libmem2.so libmem3.so

CC = gcc
OPTS1 = -c -fpic
OPTS2 = -Wall -Werror

all: libmem1.so libmem2.so libmem3.so

main: main.c libmem3.so
	$(CC) -lmem3 -L . -o main main.c $(OPTS2)

libmem1.so: mem1.o
	$(CC) -shared -o libmem1.so mem1.o  $(OPTS2)

libmem2.so: mem2.o
	$(CC) -shared -o libmem2.so mem2.o  $(OPTS2)

libmem3.so: mem3.o
	$(CC) -shared -o libmem3.so mem3.o  $(OPTS2)

mem1.o: mem1.c
	$(CC) $(OPTS1) mem1.c -o mem1.o $(OPTS2)

mem2.o: mem2.c
	$(CC) $(OPTS1) mem2.c -o mem2.o $(OPTS2)

mem3.o: mem3.c
	$(CC) $(OPTS1) mem3.c -o mem3.o $(OPTS2)

clean:
	rm -rf *.so *.o
