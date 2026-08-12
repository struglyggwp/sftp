CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread -g -Iinclude

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
BIN = $(BIN_DIR)/server

.PHONY: all run clean

all: $(BIN)

run: $(BIN)
	./$(BIN)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c include/*.h ../common/include/*.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
