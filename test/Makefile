CC      = gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -g
DEFS     = -DEMOS_DEBUG
INC      = -I..
SRC      = ../emos.c ../emos_mem.c ../emos_msg.c main.c

# Windows（含 MSYS2/Git Bash 环境）生成 .exe，Linux/WSL 生成无后缀
UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(OS),Windows_NT)
    BIN = emos.exe
else ifneq (,$(findstring MSYS,$(UNAME_S)))
    BIN = emos.exe
else ifneq (,$(findstring MINGW,$(UNAME_S)))
    BIN = emos.exe
else
    BIN = emos
endif

all: $(BIN)

$(BIN): $(SRC) ../emos.h ../emos_config.h
	$(CC) $(CFLAGS) $(DEFS) $(INC) -o $@ $(SRC)

clean:
	rm -f emos.exe emos

.PHONY: all run clean
