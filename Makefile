# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wvla -Wextra -D_GNU_SOURCE -w

# Source files
SRCS = main.c

# Target executable
TARGET = pop

# Default rule to build the executable
all: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
	@echo "Compilation successful. Executable $(TARGET) generated."
# Run the executable
run: $(TARGET)
	./$(TARGET)

# Clean rule to remove generated files
clean:
	rm -f $(TARGET)
