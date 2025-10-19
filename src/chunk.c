#include "chunk.h"

#include "defines.h"
#include "profiler.h"

typedef struct {
  GLfloat pos[3];
  GLfloat tex[2];
  GLint side_index;
  GLint block_type;
} Vertex;
// 32 * 7 = 224 bytes per vertex
// thats kinda crazy
// Could send uniform chunk pos and use only 1 byte for each block offset
// 3 bytes pos
// Texcoords could be only one bit (only 0 or 1)
// 2 bits tex
// Side index only needs up to 6, so 3 bits
// 3 bits side index
// Block type depends how many blocks there are, so could use a byte
// one byte side index
// So 4.625 bytes per vertex
// Or 37 bits per vertex
// Probably needs to be padded up to 5 bytes
// Can only pass as low as 4 bytes tho, so 8 bytes

size_t vertex_num = 4;
size_t vertex_sizes[] = {sizeof(GLfloat), sizeof(GLfloat), sizeof(GLint),
                         sizeof(GLint)};
size_t vertex_counts[] = {3, 2, 1, 1};
GLenum vertex_types[] = {GL_FLOAT, GL_FLOAT, GL_INT, GL_INT};

Chunk *create_chunk(int chunk_x, int chunk_y, int chunk_z) {
  Chunk *chunk = calloc(1, sizeof(Chunk));
  if (!chunk) {
    fprintf(stderr,
            "(create_chunk): Couldn't create chunk at position (%d, %d, %d), "
            "calloc failed.\n",
            chunk_x, chunk_y, chunk_z);
    return NULL;
  }
  chunk->coords[0] = chunk_x;
  chunk->coords[1] = chunk_y;
  chunk->coords[2] = chunk_z;
  chunk->blocks = NULL;
  chunk->num_blocks = 0;
  chunk->mesh =
      nu_create_mesh(vertex_num, vertex_sizes, vertex_counts, vertex_types);
  chunk->state = STATE_EMPTY;
#ifdef MULTITHREAD
  pthread_mutex_init(&chunk->chunk_mutex, NULL);
#endif
  return chunk;
}

// I ahve no idea why i can just return early from these
void lock_chunk(Chunk *chunk) {
  return;
  if (!chunk)
    return;
#ifdef MULTITHREAD
  pthread_mutex_lock(&chunk->chunk_mutex);
#endif
}

void unlock_chunk(Chunk *chunk) {
  return;
  if (!chunk)
    return;
#ifdef MULTITHREAD
  pthread_mutex_unlock(&chunk->chunk_mutex);
#endif
}

void destroy_chunk(Chunk **chunk) {
  if (!chunk || !(*chunk))
    return;
  lock_chunk(*chunk);
  if ((*chunk)->mesh)
    nu_destroy_mesh(&(*chunk)->mesh);
  if ((*chunk)->blocks) {
    free((*chunk)->blocks);
    (*chunk)->blocks = NULL;
  }
  unlock_chunk(*chunk);
#ifdef MULTITHREAD
  pthread_mutex_destroy(&(*chunk)->chunk_mutex);
#endif
  *chunk = NULL;
}

bool chunk_set_block(Chunk *chunk, BlockType block, size_t x, size_t y,
                     size_t z) {
  if (!chunk || x >= CHUNK_WIDTH || y >= CHUNK_HEIGHT || z >= CHUNK_LENGTH)
    return false;
  if (!chunk->blocks || chunk->state == STATE_EMPTY)
    return false;
  chunk->blocks[CHUNK_INDEX(x, y, z)] = (Block){.type = block};
  return true;
}

Block *chunk_get_block(Chunk *chunk, size_t x, size_t y, size_t z) {
  if (!chunk || x >= CHUNK_WIDTH || y >= CHUNK_HEIGHT || z >= CHUNK_LENGTH)
    return NULL;
  if (!chunk->blocks || chunk->state == STATE_EMPTY)
    return NULL;
  return &chunk->blocks[CHUNK_INDEX(x, y, z)];
}

#include "chunk_gen.c"
#include "chunk_mesh.c"

