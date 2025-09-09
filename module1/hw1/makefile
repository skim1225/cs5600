# Target executable name
TARGET = SoojiK.CS5600.LearnC

# Compiler
CC = clang

# Compiler flags
CFLAGS = -Wall -Wextra -std=c11

# Source files
SRCS = SoojiK.CS5600.LearnC.c mpg2km.c

# Object files
OBJS = $(SRCS:.c=.o)

# Build rule
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Clean rule
clean:
	rm -f $(TARGET) $(OBJS)
