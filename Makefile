# Define the compiler
CC = gcc

# Define the compiler flags
CFLAGS = -O3 -Wall

# Define the source files
SRCS = main.c server.c handlers.c

# Define the executable name
EXEC = ftp_server

# Default target
all: $(EXEC)

# Rule to compile and link in one go
$(EXEC): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

# Clean up the build files
clean:
	rm -f $(EXEC)

# Phony targets
.PHONY: all clean

