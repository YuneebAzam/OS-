CC := gcc

# Directories
SRC_DIR := src
INCLUDE_DIR := include
OBJ_DIR := obj

# Compiler flags
CFLAGS := -I$(INCLUDE_DIR) -pthread -Wall -Wextra -Werror

# Linker flags
LDFLAGS := -lcurl -lpthread

# Target executable
TARGET := file_download_manager

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.c)
HEADERS := $(wildcard $(INCLUDE_DIR)/*.h)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean

# Default target
all: $(TARGET)

# Link objects into the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile source files into object files in obj directory
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
