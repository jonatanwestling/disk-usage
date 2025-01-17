#Makefile for mdu.c
CC = gcc
CFLAGS = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition

SRC1 = mdu.c
SRC2 = jstack.c 
OUT1 = mdu.o
OUT2 = jstack.o
HDR1 = mdu.h
HDR2 = jstack.h 
EXEC = mdu

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OUT1) $(OUT2)
	$(CC) -lm -pthread $(CFLAGS) -o $(EXEC) $(OUT1) $(OUT2)

$(OUT1): $(SRC1) $(HDR1) $(HDR2)
	$(CC) $(CFLAGS) -c $(SRC1) -o $(OUT1)

$(OUT2): $(SRC2) $(HDR1) $(HDR2)
	$(CC) $(CFLAGS) -c $(SRC2) -o $(OUT2)

clean:
	rm -f $(EXEC) $(OUT1) $(OUT2)