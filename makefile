CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS = 

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Lista explícita de archivos fuente
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/parse.c \
          $(SRC_DIR)/execute.c \
          $(SRC_DIR)/builtin.c \
          $(SRC_DIR)/signals.c \
          $(SRC_DIR)/logger.c

OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/myshell

.PHONY: all clean run valgrind test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) myshell.log

run: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./$(TARGET)

test: $(TARGET)
	@echo "=== Testing myshell ==="
	@echo "pwd" | ./$(TARGET) 2>&1 | tail -1
	@echo "help" | ./$(TARGET) 2>&1 | head -5
	@echo "exit" | ./$(TARGET) 2>&1 >/dev/null
	@echo "Tests completed"

debug: $(TARGET)
	gdb ./$(TARGET)

print:
	@echo "SOURCES: $(SOURCES)"
	@echo "OBJECTS: $(OBJECTS)"
	@echo "TARGET: $(TARGET)"