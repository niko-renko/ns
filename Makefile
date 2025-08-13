CC = gcc
CFLAGS = -Wall -Wextra -Wunused -O2
COMMON = set/*.c cmd/*.c kbd/*.c state/*.c common.c

init:
	$(CC) $(CFLAGS) main_init.c $(COMMON) -o bin/init

cmd:
	$(CC) $(CFLAGS) main_cmd.c $(COMMON) -o bin/cmd

clean:
	rm -f bin/*
