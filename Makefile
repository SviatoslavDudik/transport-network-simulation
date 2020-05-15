CC=gcc
EXE=main

INC_DIR=include
OBJ_DIR=obj
SRC_DIR=src
CFLAGS=-I$(INC_DIR) -Wall -std=c99
LDLIBS=-lpthread

SRC=$(wildcard $(SRC_DIR)/*.c)
OBJ=$(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS=$(wildcard $(INC_DIR)/*.h)

.PHONY=all clean tags doc

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(LDLIBS) $^ -o $@
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJ)
tags: $(SRC) $(DEPS)
	ctags $(SRC) $(DEPS)
doc: $(SRC) $(DEPS)
	doxygen doc/conf
