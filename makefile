CC = gcc

SRC_DIR = src
NU_GL_DIR = nuGL2
BUILD_DIR = build

SRCS = $(SRC_DIR)/*.c $(NU_GL_DIR)/nuGL.c
TARGET = main

CFLAGS = -Wall -I. -I$(NU_GL_DIR) -O3

linux:
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRCS) $(CFLAGS) -o $(BUILD_DIR)/$(TARGET) -lGL -lglfw -lGLEW -lm

run: linux
	$(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)/*

