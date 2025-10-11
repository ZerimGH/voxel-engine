#ifndef WORLD_H

#define WORLD_H

#include "block.h"
#include "chunk.h"
#include "nuGL.h"
#include <cglm/cglm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
  nu_Program *program;
  nu_Texture *block_textures;
  ChunkMap map;
  size_t rdx, rdy, rdz; //render distances in each axis 
  int cx, cy, cz;       // the centre of the world (where chunks load around)
} World;

World *create_world();
void destroy_world(World **world);
void render_world(World *world, mat4 vp);
void world_update_centre(World *world, int nx, int ny, int nz);

#endif
