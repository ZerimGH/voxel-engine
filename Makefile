TARGET = ./main

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -O3

INCLUDE_DIRS = nuGL2 external
INCLUDE_DIRS += $(shell find src -type d)
CFLAGS += $(addprefix -I, $(INCLUDE_DIRS))
SRCS = $(shell find src -name '*.c')
SRCS += nuGL2/nuGL.c
OBJS = $(SRCS:.c=.o)

LD = gcc 
LDFLAGS = -lglfw -lGL -lGLEW -lm

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean: 
	# rm -f $(OBJS)
	$(shell find src -name '*.o' -delete)
	rm -f $(TARGET)

.PHONY: run
run: $(TARGET)
	$(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(TARGET)

