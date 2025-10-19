#include "world.h"
#include "player.h"

static void world_load_chunks(World *world);
static ChunkNode *hashmap_get(World *world, int x, int y, int z);
bool world_update_queue(World *world);

static inline void world_lock_bucket(World *world, size_t bucket) {
  if (!world || bucket >= HASHMAP_SIZE)
    return;
#ifdef MULTITHREAD
  pthread_mutex_lock(&world->map.bucket_mutexes[bucket]);
#endif
}

static inline void world_unlock_bucket(World *world, size_t bucket) {
  if (!world || bucket >= HASHMAP_SIZE)
    return;
#ifdef MULTITHREAD
  pthread_mutex_unlock(&world->map.bucket_mutexes[bucket]);
#endif
}

static inline void world_lock_queue(World *world) {
  if (!world)
    return;
#ifdef MULTITHREAD
  pthread_mutex_lock(&world->queue_mutex);
#endif
}

static inline void world_unlock_queue(World *world) {
  if (!world)
    return;
#ifdef MULTITHREAD
  pthread_mutex_unlock(&world->queue_mutex);
#endif
}

#ifdef MULTITHREAD
void *thread_routine(void *arg) {
  World *world = (World *)arg;
  if (!world)
    return NULL;
  while (!world->kill) {
    world_update_queue(world); 
    usleep(1000); // Without this, theres a weird slowdown when breaking or
                  // placing blocks.
  }
  return NULL;
}
#endif

World *create_world(uint32_t world_seed) {
  // Create the world's shader program
  nu_Program *program =
      nu_create_program(2, "shaders/block.vert", "shaders/block.frag");
  if (!program) {
    fprintf(stderr, "(create_world): Error creating world, nu_create_program() "
                    "returned NULL.\n");
    return NULL;
  }
  nu_register_uniform(program, "uMVP", GL_FLOAT_MAT4);
  nu_register_uniform(program, "uPlayerPos", GL_FLOAT_VEC3);
  nu_register_uniform(program, "uRenderDistance", GL_FLOAT);

  // Load the texture array for blocks
  nu_Texture *block_textures =
      nu_load_texture_array(NUM_BLOCK_TEXTURES, BLOCK_TEXTURES);
  if (!block_textures) {
    fprintf(stderr, "(create_world): Error creating world, "
                    "nu_load_texture_array() returned NULL.\n");
    nu_destroy_program(&program);
    return NULL;
  }

  // Allocate world
  World *world = calloc(1, sizeof(World));
  if (!world) {
    fprintf(stderr, "(create_world): Error creating world, calloc failed.\n");
    nu_destroy_program(&program);
    nu_destroy_texture(&block_textures);
    return NULL;
  }

  // Set members
  world->program = program;
  world->block_textures = block_textures;
  world->queue.items = NULL;
  world->queue.items_alloced = 0;
  world->queue.num_items = 0;

#ifdef MULTITHREAD
  pthread_create(&world->chunk_thread, NULL, thread_routine, (void *)world);
  // pthread_mutex_init(&world->hashmap_mutex, NULL);
  for (size_t i = 0; i < HASHMAP_SIZE; i++) {
    pthread_mutex_init(&world->map.bucket_mutexes[i], NULL);
  }
  pthread_mutex_init(&world->queue_mutex, NULL);
  world->kill = false;
#endif

  // Set world centre and render distance
  world->cx = 0;
  world->cy = 0;
  world->cz = 0;
  world->rdx = RENDER_DISTANCE;
  world->rdy = RENDER_DISTANCE;
  world->rdz = RENDER_DISTANCE;
  world->seed = world_seed;
  // Queue initial chunks
  world_load_chunks(world);

  return world;
}

void destroy_world(World **world) {
  if (!world || !(*world))
    return;
  // Destroy rendering resources
  nu_destroy_program(&(*world)->program);
  nu_destroy_texture(&(*world)->block_textures);

#ifdef MULTITHREAD
  // Stop thread on world destroyed
  (*world)->kill = true;
  pthread_join((*world)->chunk_thread, NULL);
  // pthread_mutex_destroy(&(*world)->hashmap_mutex);
  for (size_t i = 0; i < HASHMAP_SIZE; i++) {
    pthread_mutex_destroy(&(*world)->map.bucket_mutexes[i]);
  }
  pthread_mutex_destroy(&(*world)->queue_mutex);
#endif

  // Destroy every loaded chunk
  for (size_t i = 0; i < HASHMAP_SIZE; i++) {
    ChunkNode *node = (*world)->map.buckets[i];
    while (node) {
      ChunkNode *next = node->next;
      destroy_chunk(&node->chunk);
      free(node);
      node = next;
    }
  }

  // Free queue
  if ((*world)->queue.items)
    free((*world)->queue.items);

  free(*world);
  *world = NULL;
  return;
}

// Append a list of chunk coordinates to the queue
static bool world_queue_chunk(World *world, int x, int y, int z) {
  if (!world)
    return false;
  world_lock_queue(world);
  // If not allocated, allocate
  if (!world->queue.items || world->queue.items_alloced == 0) {
    if (world->queue.items)
      free(world->queue.items);
    world->queue.items_alloced = 1024;
    world->queue.items = calloc(world->queue.items_alloced, sizeof(QueueItem));
    world->queue.num_items = 0;
  }

  // If queue is too small, double size
  if (world->queue.num_items >= world->queue.items_alloced) {
    world->queue.items_alloced *= 2;
    QueueItem *new_items = realloc(
        world->queue.items, sizeof(QueueItem) * world->queue.items_alloced);
    if (!new_items) {
      world_unlock_queue(world);
      return false;
    }
    world->queue.items = new_items;
  }

  // Append queue item
  world->queue.items[world->queue.num_items++] =
      (QueueItem){.x = x, .y = y, .z = z};
  world_unlock_queue(world);

  return true;
}

// Pop from the queue, and generate + mesh that chunk
bool world_update_queue(World *world) {
  if (!world || !world->queue.items || world->queue.items_alloced == 0 ||
      world->queue.num_items == 0)
    return false;

  world_lock_queue(world);

  // Check if queue can shrink
  if (world->queue.num_items < world->queue.items_alloced / 2 &&
      world->queue.items_alloced > 1024) {
    world->queue.items_alloced /= 2;
    QueueItem *new_items =
        realloc(world->queue.items,
                sizeof(QueueItem) * world->queue.items_alloced); // Shrink queue
    if (!new_items) {
      // Handle realloc failure
      world_unlock_queue(world);
      return false;
    }
    world->queue.items = new_items;
  }

  // Pop from queue
  if (world->queue.num_items == 0) {
    world_unlock_queue(world);
    return false;
  }
  /*
  QueueItem item;
  item = world->queue.items[--world->queue.num_items];
  */
  int min_dist_sqrd = INT_MAX;
  int min_idx = 0;
  for (size_t i = 0; i < world->queue.num_items; i++) {
    QueueItem item = world->queue.items[i];
    int dx = item.x - world->cx;
    int dy = item.y - world->cy;
    int dz = item.z - world->cz;
    int dist_sqrd = dx * dx + dy * dy + dz * dz;
    if (dist_sqrd < min_dist_sqrd) {
      min_dist_sqrd = dist_sqrd;
      min_idx = i;
    }
  }
  QueueItem item = world->queue.items[min_idx];
  world->queue.items[min_idx] = world->queue.items[world->queue.num_items - 1];
  world_unlock_queue(world);

  // If chunk is not loaded, exit early
  ChunkNode *node = hashmap_get(world, item.x, item.y, item.z);
  if (!node)
    return false;
  Chunk *chunk = node->chunk;
  if (!chunk)
    return false; // This should never happen

  // Mesh and generate the chunk
  lock_chunk(chunk);
  generate_chunk(chunk, world->seed);
  mesh_chunk(chunk);
  unlock_chunk(chunk);
  return true;
}

// Render every loaded chunk, and send meshed chunks to GPU
void render_world(World *world, void *p, float aspect) {
  if (!world || !p)
    return;

  Player *player = (Player *)p;
  nu_set_uniform(world->program, "uPlayerPos", player->position);
  float render_dist = world->rdx * CHUNK_WIDTH;
  nu_set_uniform(world->program, "uRenderDistance", &render_dist);

  // Set OpenGL parameters
  glEnable(GL_DEPTH_TEST);
#ifdef GREEDY
  glDisable(GL_CULL_FACE); // Greedy meshing bug isnt visible without backface
#else                      // culling
  glEnable(GL_CULL_FACE);
#endif

  // Use and upload VP matrix to program
  mat4 vp;
  camera_calculate_vp_matrix(player->camera, vp, aspect);
  nu_use_program(world->program);
  nu_set_uniform(world->program, "uMVP", &vp[0][0]);
  nu_bind_texture(world->block_textures, 0);

  for (size_t i = 0; i < HASHMAP_SIZE; i++) {
    world_lock_bucket(world, i);
    ChunkNode *node = world->map.buckets[i];
    while (node) {
      ChunkNode *next = node->next;
      Chunk *chunk = node->chunk;
      if (chunk && chunk->mesh) {
        lock_chunk(chunk);
        ChunkState state = chunk->state;
        // If the chunk needs to be sent, send it
        if (state == STATE_NEEDS_SEND) {
          nu_send_mesh(chunk->mesh);
          nu_free_mesh(chunk->mesh);
          chunk->state = STATE_DONE;
        }
        // Render
        nu_render_mesh(chunk->mesh);
        unlock_chunk(chunk);
      }
      node = next;
    }
    world_unlock_bucket(world, i);
  }
}

static inline uint32_t hash_chunk_coords(int x, int y, int z) {
  return (uint32_t)(x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
}

static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

static uint32_t get_bucket(int x, int y, int z) {
  uint32_t bucket = hash_chunk_coords(x, y, z) % HASHMAP_SIZE;
  return bucket;
}

static ChunkNode *hashmap_get(World *world, int x, int y, int z) {
  if (!world)
    return NULL;
  uint32_t bucket = get_bucket(x, y, z);
  world_lock_bucket(world, bucket);
  ChunkNode *node = world->map.buckets[bucket];
  while (node) {
    if (node->x == x && node->y == y && node->z == z) {
      world_unlock_bucket(world, bucket);
      return node;
    }
    node = node->next;
  }
  world_unlock_bucket(world, bucket);
  return NULL;
}

/*
static void hashmap_remove(World *world, int x, int y, int z) {
  if (!world) return;
  uint32_t bucket = get_bucket(x, y, z);
  world_lock_bucket(world, bucket);
  ChunkNode *prev = NULL, *node = world->map.buckets[bucket];
  while (node) {
    if (node->x == x && node->y == y && node->z == z) {
      if (prev)
        prev->next = node->next;
      else
        world->map.buckets[bucket] = node->next;

      destroy_chunk(&node->chunk);
      world_unlock_bucket(world, bucket);
      free(node);
      return;
    }
    prev = node;
    node = node->next;
  }
  world_unlock_bucket(world, bucket);
}
*/

static void hashmap_append(World *world, Chunk *chunk) {
  if (!world || !chunk ||
      hashmap_get(world, chunk->coords[0], chunk->coords[1],
                  chunk->coords[2])) {
    if (chunk)
      destroy_chunk(&chunk);
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

  world_lock_bucket(world, bucket);

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
  world_unlock_bucket(world, bucket);
}

// Create chunks that are in render distance, if not already loaded
static void world_load_chunks(World *world) {
  if (!world)
    return;
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

// Destroy chunks that are out of render distance
static void world_unload_chunks(World *world) {
  if (!world)
    return;
  size_t count = 0;

  for (size_t i = 0; i < HASHMAP_SIZE; i++) {
    world_lock_bucket(world, i);
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
    world_unlock_bucket(world, i);
  }
}

// Update the position that chunks load around
void world_update_centre(World *world, int nx, int ny, int nz) {
  if (!world)
    return;
  if (nx == world->cx && ny == world->cy && nz == world->cz)
    return;
  world->cx = nx;
  world->cy = ny;
  world->cz = nz;
  world_load_chunks(world);
  world_unload_chunks(world);
}

Block *world_get_block(World *world, int x, int y, int z) {
  if (!world)
    return NULL;
  int cx = (int)floorf((float)x / (float)CHUNK_WIDTH);
  int cy = (int)floorf((float)y / (float)CHUNK_HEIGHT);
  int cz = (int)floorf((float)z / (float)CHUNK_LENGTH);
  ChunkNode *node = hashmap_get(world, cx, cy, cz);
  if (!node)
    return NULL;
  Chunk *chunk = node->chunk;
  if (!chunk)
    return NULL;
  size_t ccx = x - (cx * CHUNK_WIDTH);
  size_t ccy = y - (cy * CHUNK_HEIGHT);
  size_t ccz = z - (cz * CHUNK_LENGTH);
  lock_chunk(chunk);
  Block *block = chunk_get_block(chunk, ccx, ccy, ccz);
  unlock_chunk(chunk);
  return block;
}

void world_set_block(World *world, BlockType block, int x, int y, int z) {
  if (!world)
    return;
  int cx = (int)floorf((float)x / (float)CHUNK_WIDTH);
  int cy = (int)floorf((float)y / (float)CHUNK_HEIGHT);
  int cz = (int)floorf((float)z / (float)CHUNK_LENGTH);
  ChunkNode *node = hashmap_get(world, cx, cy, cz);
  if (!node)
    return;
  Chunk *chunk = node->chunk;
  if (!chunk)
    return;
  size_t ccx = x - (cx * CHUNK_WIDTH);
  size_t ccy = y - (cy * CHUNK_HEIGHT);
  size_t ccz = z - (cz * CHUNK_LENGTH);
  lock_chunk(chunk);
  bool success = chunk_set_block(chunk, block, ccx, ccy, ccz);
  unlock_chunk(chunk);
  if (success) {
    chunk->state = STATE_NEEDS_MESH;
    world_queue_chunk(world, cx, cy, cz);
  }
}

Block *world_get_blockf(World *world, float x, float y, float z) {
  int ix = (int)floorf(x);
  int iy = (int)floorf(y);
  int iz = (int)floorf(z);
  return world_get_block(world, ix, iy, iz);
}

void world_set_blockf(World *world, BlockType block, float x, float y,
                      float z) {
  int ix = (int)floorf(x);
  int iy = (int)floorf(y);
  int iz = (int)floorf(z);
  world_set_block(world, block, ix, iy, iz);
}

RayCastReturn world_raycast(World *world, float x, float y, float z, float dx,
                            float dy, float dz, float max_dist) {
  RayCastReturn ret = {0};
  if (!world)
    return ret;
  // Normalise direction vector
  float d_mag = (dx * dx + dy * dy + dz * dz);
  if (d_mag == 0.f)
    return ret; // Handle 0 magnitude
  dx /= d_mag;
  dy /= d_mag;
  dz /= d_mag;
  // Get integer position of first block
  int cur_x = (int)floorf(x);
  int cur_y = (int)floorf(y);
  int cur_z = (int)floorf(z);

  int last_x = (int)floorf(x);
  int last_y = (int)floorf(y);
  int last_z = (int)floorf(z);
  float dist = 0.f;

  float tdx = fabsf(1.f / dx);
  float tdy = fabsf(1.f / dy);
  float tdz = fabsf(1.f / dz);

  int step_x = (dx > 0) ? 1 : -1;
  int step_y = (dy > 0) ? 1 : -1;
  int step_z = (dz > 0) ? 1 : -1;

  float tmx = (dx > 0) ? ((cur_x + 1 - x) / dx) : ((x - cur_x) / -dx);
  float tmy = (dy > 0) ? ((cur_y + 1 - y) / dy) : ((y - cur_y) / -dy);
  float tmz = (dz > 0) ? ((cur_z + 1 - z) / dz) : ((z - cur_z) / -dz);

  while (dist < max_dist) {
    Block *block = world_get_block(world, cur_x, cur_y, cur_z);
    if (block && block->type != BlockAir) {
      ret.hit = true;
      ret.hit_x = cur_x;
      ret.hit_y = cur_y;
      ret.hit_z = cur_z;
      ret.last_x = last_x;
      ret.last_y = last_y;
      ret.last_z = last_z;
      ret.block_hit = block;
      return ret;
    }

    last_x = cur_x;
    last_z = cur_z;
    last_y = cur_y;

    if (tmx < tmy) {
      if (tmx < tmz) {
        cur_x += step_x;
        dist = tmx;
        tmx += tdx;
      } else {
        cur_z += step_z;
        dist = tmz;
        tmz += tdz;
      }
    } else {
      if (tmy < tmz) {
        cur_y += step_y;
        dist = tmy;
        tmy += tdy;
      } else {
        cur_z += step_z;
        dist = tmz;
        tmz += tdz;
      }
    }
  }
  return ret;
}
