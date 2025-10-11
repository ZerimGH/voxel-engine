#ifndef CHUNK_H

#define CHUNK_H

#define CHUNK_WIDTH 32
#define CHUNK_HEIGHT 32
#define CHUNK_LENGTH 32

#define CHUNK_AREA (CHUNK_WIDTH * CHUNK_LENGTH)
#define CHUNK_VOLUME (CHUNK_AREA * CHUNK_HEIGHT)

#define CHUNK_INDEX(x, y, z) ((z) + (x) * CHUNK_LENGTH + (y) * CHUNK_AREA)

// Includes
#include "block.h"
#include "defines.h"
#include "noise.h"
#define NUGL_DEBUG
#include "nuGL.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef MULTITHREAD
#include <pthread.h>
#endif

typedef enum { STATE_EMPTY, STATE_NEEDS_MESH, STATE_NEEDS_SEND, STATE_DONE } ChunkState;

// Structs
typedef struct {
  int coords[3];
  Block *blocks;
  size_t num_blocks;
  nu_Mesh *mesh;
  ChunkState state;
#ifdef MULTITHREAD
  pthread_mutex_t chunk_mutex;
#endif
} Chunk;

// Function prototypes
Chunk *create_chunk(int chunk_x, int chunk_y, int chunk_z);
void print_chunk(Chunk *chunk);
void destroy_chunk(Chunk **chunk);
void generate_chunk(Chunk *chunk);
void mesh_chunk(Chunk *chunk);

#endif
