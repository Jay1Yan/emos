CC      = gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -g
DEFS     = -DEMOS_DEBUG
INC      = -I..
SRC      = ../emos.c ../emos_mem.c ../emos_msg.c main.c
SRC_TEST = ../emos.c ../emos_mem.c ../emos_msg.c test_auto.c

BIN      = emos
BIN_TEST = emos_test

all: $(BIN)

$(BIN): $(SRC) ../emos.h ../emos_config.h ../emos_mem.h ../emos_msg.h
	$(CC) $(CFLAGS) $(DEFS) $(INC) -o $@ $(SRC)

test: $(BIN_TEST)
	./$(BIN_TEST)

$(BIN_TEST): $(SRC_TEST) ../emos.h ../emos_config.h ../emos_mem.h ../emos_msg.h
	$(CC) $(CFLAGS) $(DEFS) $(INC) -o $@ $(SRC_TEST)

clean:
	rm -f emos emos_test

.PHONY: all run test clean
