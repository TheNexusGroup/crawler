CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -g -O2
LDFLAGS = -lm -pthread

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.c $(SRCDIR)/*/*.c)
HEADERS = $(wildcard $(INCDIR)/*.h $(SRCDIR)/*.h $(SRCDIR)/*/*.h)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Target executable
TARGET = $(BINDIR)/crawler_test_c

# Default target
all: directories $(TARGET)

# Create directories if they don't exist
directories:
	@mkdir -p $(OBJDIR) $(OBJDIR)/database $(OBJDIR)/services $(OBJDIR)/utils $(BINDIR)

# Main target
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete!"

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -I$(INCDIR) -I$(SRCDIR) -c $< -o $@

# Clean build files
clean:
	@rm -rf $(OBJDIR) $(BINDIR)
	@echo "Clean complete!"

# Run the program
run: $(TARGET)
	@./$(TARGET)

# Install dependencies (if needed)
install-deps:
	@echo "No external dependencies required"

# Debug build
debug: CFLAGS += -DDEBUG -g3
debug: $(TARGET)

# Release build
release: CFLAGS += -DNDEBUG -O3
release: clean $(TARGET)

# Check for memory leaks with valgrind
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

.PHONY: all clean run install-deps debug release directories memcheck