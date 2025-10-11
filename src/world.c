#include "world.h"

static void world_load_chunk(World *world, int x, int y, int z);
static void world_load_chunks(World *world);
static ChunkNode *hashmap_get(World *world, int x, int y, int z);
bool world_update_queue(World *world);

#ifdef MULTITHREAD
void *thread_routine(void *arg) {
  World *world = (World *)arg;
  if (!world) return NULL;
  while (!world->kill) {
    if (!world_update_queue(world)) {
      usleep(1000);
    }
  }
  return NULL;
}
#endif

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

  world->queue.items = NULL;
  world->queue.items_alloced = 0;
  world->queue.num_items = 0;

#ifdef MULTITHREAD
  pthread_create(&world->chunk_thread, NULL, thread_routine, (void *)world);
  pthread_mutex_init(&world->hashmap_mutex, NULL);
  pthread_mutex_init(&world->queue_mutex, NULL);
  world->kill = false;
#endif

  // Queue a few initial chunks
  world->cx = 0;
  world->cy = 0;
  world->cz = 0;
  world->rdx = 8;
  world->rdy = 4;
  world->rdz = 8;

  world_load_chunks(world);

  return world;
}

static bool world_queue_chunk(World *world, int x, int y, int z) {
  if (!world) return false;
#ifdef MULTITHREAD
  pthread_mutex_lock(&world->queue_mutex);
#endif
  if (!world->queue.items || world->queue.items_alloced == 0) {
    if (world->queue.items) free(world->queue.items);
    world->queue.items_alloced = 1024;
    world->queue.items = calloc(world->queue.items_alloced, sizeof(QueueItem));
    world->queue.num_items = 0;
  }

  if (world->queue.num_items >= world->queue.items_alloced) {
    world->queue.items_alloced *= 2;
    QueueItem *new_items = realloc(world->queue.items, sizeof(QueueItem) * world->queue.items_alloced);
    if (!new_items) {
#ifdef MULTITHREAD
      pthread_mutex_unlock(&world->queue_mutex);
#endif
      return false;
    }
    world->queue.items = new_items;
  }

  world->queue.items[world->queue.num_items++] = (QueueItem){.x = x, .y = y, .z = z};

#ifdef MULTITHREAD
  pthread_mutex_unlock(&world->queue_mutex);
#endif

  return true;
}

bool world_update_queue(World *world) {
  if (!world || !world->queue.items || world->queue.items_alloced == 0 || world->queue.num_items == 0) return false;

  QueueItem item;

#ifdef MULTITHREAD
  pthread_mutex_lock(&world->queue_mutex);
#endif

  // Check if queue can shrink
  if(world->queue.num_items < world->queue.items_alloced / 2 && world->queue.items_alloced > 1024) {
    world->queue.items_alloced /= 2;
    QueueItem *new_items = realloc(world->queue.items, sizeof(QueueItem) * world->queue.items_alloced); 
    if(!new_items) return false;
    world->queue.items = new_items;
  }

  if (world->queue.num_items == 0) {
#ifdef MULTITHREAD
    pthread_mutex_unlock(&world->queue_mutex);
#endif
    return false;
  }
  item = world->queue.items[--world->queue.num_items];

#ifdef MULTITHREAD
  pthread_mutex_unlock(&world->queue_mutex);
#endif

  ChunkNode *node = hashmap_get(world, item.x, item.y, item.z);
  if (!node) return false;

  Chunk *chunk = node->chunk;

  if (!chunk) return false;

#ifdef MULTITHREAD
  pthread_mutex_lock(&chunk->chunk_mutex);
#endif
  generate_chunk(chunk);
  mesh_chunk(chunk);

#ifdef MULTITHREAD
  pthread_mutex_unlock(&chunk->chunk_mutex);
#endif

  return true;
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

  if ((*world)->queue.items) free((*world)->queue.items);

  #ifdef MULTITHREAD
  (*world)->kill = true;
  pthread_join((*world)->chunk_thread, NULL);
  pthread_mutex_destroy(&(*world)->hashmap_mutex);
  pthread_mutex_destroy(&(*world)->queue_mutex);
  #endif

  free(*world);
  *world = NULL;
  return;
}

void render_world(World *world, mat4 vp) {
  if (!world) return;

  glEnable(GL_DEPTH_TEST);
  #ifdef GREEDY 
  glDisable(GL_CULL_FACE);  // Greedy meshing bug isnt visible without backface
  #else                     // culling
  glEnable(GL_CULL_FACE);
  #endif

  nu_use_program(world->program);
  nu_set_uniform(world->program, "uMVP", &vp[0][0]);

#ifdef MULTITHREAD
  pthread_mutex_lock(&world->hashmap_mutex);
#endif

  for (size_t i = 0; i < HASHMAP_SIZE; i++) {
    ChunkNode *node = world->map.buckets[i];
    while (node) {
      ChunkNode *next = node->next;
      Chunk *chunk = node->chunk;
      if (chunk && chunk->mesh) {
#ifdef MULTITHREAD
        pthread_mutex_lock(&chunk->chunk_mutex);
        ChunkState state = chunk->state;
#else
        ChunkState state = chunk->state;
#endif
        if (state == STATE_NEEDS_SEND) {
          nu_send_mesh(chunk->mesh);
          nu_free_mesh(chunk->mesh);
          chunk->state = STATE_DONE;
        }
        nu_render_mesh(chunk->mesh);
#ifdef MULTITHREAD
        pthread_mutex_unlock(&chunk->chunk_mutex);
#endif
      }
      node = next;
    }
  }

#ifdef MULTITHREAD
  pthread_mutex_unlock(&world->hashmap_mutex);
#endif
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
#ifdef MULTITHREAD
  pthread_mutex_lock(&world->hashmap_mutex);
#endif
  ChunkNode *node = world->map.buckets[bucket];
  while (node) {
    if (node->x == x && node->y == y && node->z == z) {
#ifdef MULTITHREAD
      pthread_mutex_unlock(&world->hashmap_mutex);
#endif
      return node;
    }
    node = node->next;
  }
#ifdef MULTITHREAD
  pthread_mutex_unlock(&world->hashmap_mutex);
#endif
  return NULL;
}

static void hashmap_remove(World *world, int x, int y, int z) {
  if (!world) return;
  uint32_t bucket = get_bucket(x, y, z);
#ifdef MULTITHREAD
  pthread_mutex_lock(&world->hashmap_mutex);
#endif
  ChunkNode *prev = NULL, *node = world->map.buckets[bucket];
  while (node) {
    if (node->x == x && node->y == y && node->z == z) {
      if (prev)
        prev->next = node->next;
      else
        world->map.buckets[bucket] = node->next;

      destroy_chunk(&node->chunk);
      free(node);
#ifdef MULTITHREAD
      pthread_mutex_unlock(&world->hashmap_mutex);
#endif
      return;
    }
    prev = node;
    node = node->next;
  }
#ifdef MULTITHREAD
  pthread_mutex_unlock(&world->hashmap_mutex);
#endif
}

static void hashmap_append(World *world, Chunk *chunk) {
  if (!world || !chunk || hashmap_get(world, chunk->coords[0], chunk->coords[1], chunk->coords[2])) {
    if (chunk) destroy_chunk(&chunk);
    return;
  }
  int x = chunk->coords[0];
  int y = chunk->coords[1];
  int z = chunk->coords[2];

  // Create node
  ChunkNode *new_node = calloc(1, sizeof(ChunkNode));
  if (!new_node) {
    destroy_chunk(&chunk);
    return;
  }
  new_node->x = x;
  new_node->y = y;
  new_node->z = z;
  new_node->chunk = chunk;
  new_node->next = NULL;

  uint32_t bucket = get_bucket(x, y, z);

#ifdef MULTITHREAD
  pthread_mutex_lock(&world->hashmap_mutex);
#endif

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
#ifdef MULTITHREAD
  pthread_mutex_unlock(&world->hashmap_mutex);
#endif
}

/*
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
*/

// Super simple for now, allocate and queue every chunk in render distance
static void world_load_chunks(World *world) {
  if (!world) return;
  for (int x = (int)-world->rdx; x <= (int)world->rdx; x++) {
    for (int y = (int)-world->rdy; y <= (int)world->rdy; y++) {
      for (int z = (int)-world->rdz; z <= (int)world->rdz; z++) {
        int gx = world->cx + x;
        int gy = world->cy + y;
        int gz = world->cz + z;
        if (!hashmap_get(world, gx, gy, gz)) {
          Chunk *chunk = create_chunk(gx, gy, gz);
          if (world_queue_chunk(world, gx, gy, gz)) {
            hashmap_append(world, chunk);
          } else {
            destroy_chunk(&chunk);
          }
        }
      }
    }
  }
}

static void world_unload_chunks(World *world) {
  if (!world) return;
  size_t count = 0;
#ifdef MULTITHREAD
  pthread_mutex_lock(&world->hashmap_mutex);
#endif
  for (size_t i = 0; i < HASHMAP_SIZE; i++) {
    ChunkNode *node = world->map.buckets[i];
    ChunkNode *prev = NULL;
    while (node) {
      if (node->chunk) {
        int dx = abs(node->chunk->coords[0] - world->cx);
        int dy = abs(node->chunk->coords[1] - world->cy);
        int dz = abs(node->chunk->coords[2] - world->cz);
        if (dx > world->rdx || dy > world->rdy || dz > world->rdz) {
          ChunkNode *next = node->next;
          if (prev) {
            prev->next = next;
          } else {
            world->map.buckets[i] = next;
          }

          destroy_chunk(&node->chunk);
          free(node);
          node = next;
          count++;
          continue;
        }
      }
      prev = node;
      node = node->next;
    }
  }
#ifdef MULTITHREAD
  pthread_mutex_unlock(&world->hashmap_mutex);
#endif
}

void world_update_centre(World *world, int nx, int ny, int nz) {
  if (!world) return;
  if (nx == world->cx && ny == world->cy && nz == world->cz) return;
  world->cx = nx;
  world->cy = ny;
  world->cz = nz;
  world_load_chunks(world);
  world_unload_chunks(world);
}
