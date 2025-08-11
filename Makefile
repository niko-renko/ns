CC = gcc
CFLAGS = -Wall -Wextra -Wunused -O2 -ludev
COMMON = set/*.c cmd/*.c ctl/*.c common.c

init:
	$(CC) $(CFLAGS) main.c $(COMMON) -o bin/init

cmd:
	$(CC) $(CFLAGS) console_cmd/main.c $(COMMON) -o bin/cmd

clean:
	rm -f bin/*
