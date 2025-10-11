#include "world.h"

static void world_load_chunk(World *world, int x, int y, int z);

World *create_world() {
  nu_Program *program = nu_create_program(2, "shaders/block.vert", "shaders/block.frag");
  if (!program) {
    fprintf(stderr, "(create_world): Error creating world, nu_create_program() returned NULL.\n");
    return NULL;
  }
  nu_register_uniform(program, "uMVP", GL_FLOAT_MAT4);
  nu_Texture *block_textures = nu_load_texture_array(NUM_BLOCK_TEXTURES, BLOCK_TEXTURES);
  if (!block_textures) {
    fprintf(stderr, "(create_world): Error creating world, nu_load_texture_array() returned NULL.\n");
    nu_destroy_program(&program);
    return NULL;
  }

  World *world = calloc(1, sizeof(World));
  if (!world) {
    fprintf(stderr, "(create_world): Error creating world, calloc failed.\n");
    nu_destroy_program(&program);
    nu_destroy_texture(&block_textures);
    return NULL;
  }
  world->program = program;
  world->block_textures = block_textures;

  // Load a few initial chunks
  for (int x = -4; x <= 4; x++) {
    for (int y = -4; y <= 4; y++) {
      for (int z = -4; z <= 4; z++) {
        world_load_chunk(world, x, y, z);
      }
    }
  }

  return world;
}

void destroy_world(World **world) {
  if (!world || !(*world)) return;
  nu_destroy_program(&(*world)->program);
  nu_destroy_texture(&(*world)->block_textures);

  for (size_t i = 0; i < HASHMAP_SIZE; i++) {
    ChunkNode *node = (*world)->map.buckets[i];
    while (node) {
      ChunkNode *next = node->next;
      destroy_chunk(&node->chunk);
      free(node);
      node = next;
    }
  }

  free(*world);
  *world = NULL;
  return;
}

void render_world(World *world, mat4 vp) {
  if (!world) return;

  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE); // Greedy meshing bug isnt visible without backface
                           // culling

  nu_use_program(world->program);
  nu_set_uniform(world->program, "uMVP", &vp[0][0]);

  for (size_t i = 0; i < HASHMAP_SIZE; i++) {
    ChunkNode *node = world->map.buckets[i];
    while (node) {
      ChunkNode *next = node->next;
      Chunk *chunk = node->chunk;
      if (chunk && chunk->mesh) {
        nu_render_mesh(chunk->mesh);
      }
      node = next;
    }
  }
}

static inline uint32_t hash_chunk_coords(int x, int y, int z) {
  uint64_t h = (uint64_t)x * 73856093u;
  h ^= (uint64_t)y * 19349663u;
  h ^= (uint64_t)z * 83492791u;

  h ^= h >> 13;
  h *= 0x85ebca6bu;
  h ^= h >> 16;

  return (uint32_t)h;
}

static inline float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

static uint32_t get_bucket(int x, int y, int z) {
  uint32_t bucket = hash_chunk_coords(x, y, z) % HASHMAP_SIZE;
  return bucket;
}

static ChunkNode *hashmap_get(World *world, int x, int y, int z) {
  if (!world) return NULL;
  uint32_t bucket = get_bucket(x, y, z);
  ChunkNode *node = world->map.buckets[bucket];
  while (node) {
    if (node->x == x && node->y == y && node->z == z) return node;
    node = node->next;
  }
  return NULL;
}

static void hashmap_remove(World *world, int x, int y, int z) {
  if (!world) return;
  uint32_t bucket = get_bucket(x, y, z);
  ChunkNode *prev = NULL, *node = world->map.buckets[bucket];
  while (node) {
    if (node->x == x && node->y == y && node->z == z) {
      if (prev)
        prev->next = node->next;
      else
        world->map.buckets[bucket] = node->next;

      destroy_chunk(&node->chunk);
      free(node);
      return;
    }
    prev = node;
    node = node->next;
  }
}

static void hashmap_append(World *world, Chunk *chunk) {
  if (!world || !chunk || hashmap_get(world, chunk->coords[0], chunk->coords[1], chunk->coords[2])) return;
  int x = chunk->coords[0];
  int y = chunk->coords[1];
  int z = chunk->coords[2];

  // Create node
  ChunkNode *new_node = calloc(1, sizeof(ChunkNode));
  if (!new_node) {
    fprintf(stderr, "(hashmap_append): Calloc failed! Destroying chunk at (%d, %d, %d).\n", x, y, z);
    destroy_chunk(&chunk);
    return;
  }
  new_node->x = x;
  new_node->y = y;
  new_node->z = z;
  new_node->chunk = chunk;
  new_node->next = NULL;

  uint32_t bucket = get_bucket(x, y, z);
  ChunkNode *prev = world->map.buckets[bucket];
  ChunkNode *node = world->map.buckets[bucket];
  while (node) {
    prev = node;
    node = node->next;
  }

  if (prev) {
    prev->next = new_node;
  } else {
    world->map.buckets[bucket] = new_node;
  }
}

static void world_load_chunk(World *world, int x, int y, int z) {
  if (!world || hashmap_get(world, x, y, z)) return;
  Chunk *chunk = create_chunk(x, y, z);
  if (!chunk) {
    fprintf(stderr, "(world_load_chunk): Failed to load chunk at (%d, %d, %d), create_chunk() returned NULL.\n", x, y, z);
    return;
  }
  // For now, generate and mesh a chunk immediately when loaded
  generate_chunk(chunk);
  mesh_chunk(chunk);
  hashmap_append(world, chunk);
}
