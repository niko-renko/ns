CC = gcc
CFLAGS = -Wall -Wextra -Wunused -O2
COMMON = set/*.c cmd/*.c kbd/*.c state/*.c cgroup/*.c ctl/*.c common.c

init:
	$(CC) $(CFLAGS) main.c $(COMMON) -o bin/init

clean:
	rm -f bin/*
