CC = gcc

SRC_DIR = src
NU_GL_DIR = nuGL2
BUILD_DIR = build

#SRCS = $(SRC_DIR)/*.c $(NU_GL_DIR)/nuGL.c
SRCS = $(SRC_DIR)/camera.c $(SRC_DIR)/chunk.c $(SRC_DIR)/game.c $(SRC_DIR)/main.c $(SRC_DIR)/noise.c $(SRC_DIR)/player.c $(SRC_DIR)/sky.c $(SRC_DIR)/world.c $(SRC_DIR)/ui_renderer.c $(SRC_DIR)/crosshair.c $(SRC_DIR)/clouds.c $(NU_GL_DIR)/nuGL.c
TARGET = main

CFLAGS = -Wall -I. -I$(NU_GL_DIR) -Ilibs

# for linux, you need the packages:
# glfw
# glew
# pthread

linux:
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRCS) $(CFLAGS) -o $(BUILD_DIR)/$(TARGET) -lGL -lglfw -lGLEW -lm -O3 -pthread

linux-debug:
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRCS) $(CFLAGS) -o $(BUILD_DIR)/$(TARGET) -lGL -lglfw -lGLEW -lm -g -O0 -pthread

run: linux
	$(BUILD_DIR)/$(TARGET)

# for windows, you need the DLLs and headers to compile
# https://www.glfw.org/download
# https://glew.sourceforge.net/index.html
# for multithreading, you also need pthreads-win32
# https://github.com/GerHobbelt/pthread-win32
# you also need to put the dlls in the build directory to run, unless you
# figure out static compilation

windows:
	$(CC) $(SRCS) $(CFLAGS) -o $(BUILD_DIR)/$(TARGET).exe \
		-L./windlls/glfw-3.4.bin.WIN64/lib-mingw-w64/ \
		-L./windlls/glew-2.1.0/lib/Release/x64 \
		-L./windlls/pthread \
		-lglfw3 -lglew32 -lgdi32 -lopengl32 -lm -lpthread

runw: windows 
	$(BUILD_DIR)/$(TARGET).exe

clean:
	rm -rf $(BUILD_DIR)/*

