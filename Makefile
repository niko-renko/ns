CC = gcc
CFLAGS = -Wall -Wextra -O2

init:
	$(CC) $(CFLAGS) main.c set/set.c cmd/cmd.c common.c -o bin/init

cmd:
	$(CC) $(CFLAGS) cmd/main.c set/set.c cmd/cmd.c common.c -o bin/cmd

clean:
	rm -f bin/*
