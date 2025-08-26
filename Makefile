# Makefile for the 8-Ball Pool Game on Linux
# Based on the Makefile.linux example from the setup guide.

# Compiler and target executable name
CC = gcc
TARGET = pool_game

# Source files
SRCS = main.c

# Compiler flags:
# -std=c11: Use the C11 standard
# -Wall -Wextra: Enable additional warnings
# -O2: Optimization level 2
# `sdl2-config --cflags`: Get the include paths for SDL2
CFLAGS = -std=c11 -Wall -Wextra -O2 $(shell sdl2-config --cflags)

# Linker flags:
# `sdl2-config --libs`: Get the library paths and base SDL2 library
# -lSDL2_ttf: Link against the SDL2_ttf library for text rendering
# -lm: Link against the math library (for sqrt, etc.)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_ttf -lm

# Default target: build the executable
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

# Rule to clean up build files
clean:
	rm -f $(TARGET)

# Phony targets are not files
.PHONY: all clean
