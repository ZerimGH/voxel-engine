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

#ifdef MULTITHREAD
#include <pthread.h>
#include <unistd.h>
#endif

#define HASHMAP_SIZE 4096

typedef struct ChunkNode {
  Chunk *chunk;
  int x, y, z;
  struct ChunkNode *next;
} ChunkNode;

typedef struct {
  ChunkNode *buckets[HASHMAP_SIZE];
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
  nu_Program *program;
  nu_Texture *block_textures;
  ChunkMap map;
  size_t rdx, rdy, rdz; // render distances in each axis
  int cx, cy, cz;       // the centre of the world (where chunks load around)
  Queue queue;
#ifdef MULTITHREAD
  pthread_t chunk_thread;
  pthread_mutex_t hashmap_mutex;
  pthread_mutex_t queue_mutex;
  volatile bool kill;
#endif
} World;

typedef struct {
  int hit_x, hit_y, hit_z; // Store the position the ray hit at
  int last_x, last_y, last_z; // Store the last position of the ray before hit
  bool hit; // Did it hit?
  Block *block_hit; // Store the block that was hit
} RayCastReturn;

World *create_world();
void destroy_world(World **world);
void render_world(World *world, mat4 vp);
void world_update_centre(World *world, int nx, int ny, int nz);
bool world_update_queue(World *world);
Block *world_get_block(World *world, int x, int y, int z);
void world_set_block(World *world, BlockType block, int x, int y, int z);
Block *world_get_blockf(World *world, float x, float y, float z);
void world_set_blockf(World *world, BlockType block, float x, float y, float z);
RayCastReturn world_raycast(World *world, float x, float y, float z, float dx, float dy, float dz, float max_dist);

#endif
