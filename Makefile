CC = gcc
CFLAGS = -Wall -Wextra -Wunused -O2
COMMON = set/*.c cmd/cmd.c cmd/sock_cmd.c common.c

init:
	$(CC) $(CFLAGS) main.c $(COMMON) -o bin/init

cmd:
	$(CC) $(CFLAGS) cmd/main.c $(COMMON) -o bin/cmd

clean:
	rm -f bin/*
