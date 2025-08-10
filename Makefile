CC = gcc
CFLAGS = -Wall -Wextra -O2
SRC = set.c main.c cmd.c common.c
OUT = init

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
