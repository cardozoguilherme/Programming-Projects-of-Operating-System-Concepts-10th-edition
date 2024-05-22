CC = gcc
CFLAGS = -Wall -Wextra

vm: vm.o
	$(CC) $(CFLAGS) -o vm vm.o

vm.o: vm.c
	$(CC) $(CFLAGS) -c vm.c

clean:
	rm -f vm vm.o
