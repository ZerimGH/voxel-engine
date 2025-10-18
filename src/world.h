#ifndef WORLD_H

#define WORLD_H

#include "block.h"
#include "chunk.h"
#include "defines.h"
#include "nuGL.h"

#include <cglm/cglm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#ifdef MULTITHREAD
#include <pthread.h>
#include <unistd.h>
#endif

#define HASHMAP_SIZE 4096

#define RENDER_DISTANCE 8 

typedef struct ChunkNode {
  Chunk *chunk;
  int x, y, z;
  struct ChunkNode *next;
} ChunkNode;

typedef struct {
  ChunkNode *buckets[HASHMAP_SIZE];
  pthread_mutex_t bucket_mutexes[HASHMAP_SIZE];
} ChunkMap;

typedef struct {
  int x, y, z;
} QueueItem;

typedef struct {
  QueueItem *items;
  size_t items_alloced;
  size_t num_items;
} Queue;

typedef struct {
  nu_Program *program; // Shader program used to render the world
  nu_Texture *block_textures; // Texture array of block textures
  ChunkMap map; // Hashmap of loaded chunks
  size_t rdx, rdy, rdz; // render distances in each axis
  int cx, cy, cz;       // the centre of the world (where chunks load around)
  Queue queue;  // Queue of chunk coordinates to be generated and meshed
#ifdef MULTITHREAD
  pthread_t chunk_thread; // The thread that will generate and mesh chunks
  // pthread_mutex_t hashmap_mutex; // Mutex protecting hashmap lookups / insertions
  pthread_mutex_t queue_mutex; // Mutex protecting pushing and popping from the queue
  volatile bool kill; // Flag to kill the thread
#endif
} World;

typedef struct {
  bool hit; // Did it hit?
  int hit_x, hit_y, hit_z; // Store the position the ray hit at
  int last_x, last_y, last_z; // Store the last position of the ray before hit
  Block *block_hit; // Store a pointer to the block that was hit
} RayCastReturn;

// Allocate, initialise and return a pointer to a world
World *create_world();
// Destroy all of a world's resources, and null the pointer
void destroy_world(World **world);
// Render a world given a view-projection matrix
void render_world(World *world, mat4 vp);
// Set the point of the world that chunks load around
void world_update_centre(World *world, int nx, int ny, int nz);
// Generate and mesh one chunk from the queue, returns true on success, and
// false if the queue was empty
bool world_update_queue(World *world);
// Get a pointer to the block at integer coords
Block *world_get_block(World *world, int x, int y, int z);
// Set a block at integer coords
void world_set_block(World *world, BlockType block, int x, int y, int z);
// Get a pointer to a block from float coords
Block *world_get_blockf(World *world, float x, float y, float z);
// Set a block from float coords 
void world_set_blockf(World *world, BlockType block, float x, float y, float z);
// Raycast from an origin in a direction, up to a certain distance
RayCastReturn world_raycast(World *world, float x, float y, float z, float dx, float dy, float dz, float max_dist);

#endif
