#include "chunk.h"

Chunk *create_chunk(int chunk_x, int chunk_y, int chunk_z) {
  Chunk *chunk = calloc(1, sizeof(Chunk));
  if (!chunk) {
    fprintf(stderr, "(create_chunk): Couldn't create chunk at position (%d, %d, %d), calloc failed.\n", chunk_x, chunk_y, chunk_z);
    return NULL;
  }
  chunk->coords[0] = chunk_x;
  chunk->coords[1] = chunk_y;
  chunk->coords[2] = chunk_z;
  chunk->generated = false;
  chunk->blocks = NULL;
  chunk->num_blocks = 0;
  chunk->mesh = NULL;
  chunk->meshed = false;
  return chunk;
}

void destroy_chunk(Chunk **chunk) {
  if (!chunk || !(*chunk)) return;
  if ((*chunk)->mesh) nu_destroy_mesh(&(*chunk)->mesh);
  if ((*chunk)->blocks) {
    free((*chunk)->blocks);
    (*chunk)->blocks = NULL;
  }
  *chunk = NULL;
}

void generate_chunk(Chunk *chunk) {
  if (!chunk || chunk->generated) return;
  if (chunk->blocks) free(chunk->blocks);
  chunk->blocks = calloc(CHUNK_VOLUME, sizeof(Block));
  if (!chunk->blocks) {
    fprintf(stderr, "(generate_chunk): Couldn't generate chunk at coords (%d, %d, %d), calloc failed.\n", chunk->coords[0], chunk->coords[1], chunk->coords[2]);
    return;
  }
  int ccx = chunk->coords[0] * CHUNK_WIDTH;
  int ccy = chunk->coords[1] * CHUNK_HEIGHT;
  int ccz = chunk->coords[2] * CHUNK_LENGTH;
  for (size_t x = 0; x < CHUNK_WIDTH; x++) {
    int terrain_height = rand() % 10;
    for (size_t y = 0; y < CHUNK_HEIGHT; y++) {
      int gy = ccy + (int)y;
      for (size_t z = 0; z < CHUNK_LENGTH; z++) {
        BlockType block = gy <= terrain_height ? BlockSolid : BlockAir;
        size_t idx = CHUNK_INDEX(x, y, z);
        chunk->blocks[idx] = (Block){.type = block};
      }
    }
  }
  chunk->generated = true;
}

typedef struct {
  GLfloat pos[3];
  GLfloat tex[2];
} Vertex;

size_t vertex_num = 2;
size_t vertex_sizes[] = {sizeof(GLfloat), sizeof(GLfloat)};
size_t vertex_counts[] = {3, 2};
GLenum vertex_types[] = {GL_FLOAT, GL_FLOAT};

Vertex cube[] = {
    // Front face
    {{0, 0, 1}, {0, 0}},
    {{1, 0, 1}, {1, 0}},
    {{1, 1, 1}, {1, 1}},

    {{0, 0, 1}, {0, 0}},
    {{1, 1, 1}, {1, 1}},
    {{0, 1, 1}, {0, 1}},

    // Back face
    {{1, 0, 0}, {0, 0}},
    {{0, 0, 0}, {1, 0}},
    {{0, 1, 0}, {1, 1}},

    {{1, 0, 0}, {0, 0}},
    {{0, 1, 0}, {1, 1}},
    {{1, 1, 0}, {0, 1}},

    // Left face
    {{0, 0, 0}, {0, 0}},
    {{0, 0, 1}, {1, 0}},
    {{0, 1, 1}, {1, 1}},

    {{0, 0, 0}, {0, 0}},
    {{0, 1, 1}, {1, 1}},
    {{0, 1, 0}, {0, 1}},

    // Right face
    {{1, 0, 1}, {0, 0}},
    {{1, 0, 0}, {1, 0}},
    {{1, 1, 0}, {1, 1}},

    {{1, 0, 1}, {0, 0}},
    {{1, 1, 0}, {1, 1}},
    {{1, 1, 1}, {0, 1}},

    // Top face
    {{0, 1, 1}, {0, 0}},
    {{1, 1, 1}, {1, 0}},
    {{1, 1, 0}, {1, 1}},

    {{0, 1, 1}, {0, 0}},
    {{1, 1, 0}, {1, 1}},
    {{0, 1, 0}, {0, 1}},

    // Bottom face
    {{0, 0, 0}, {0, 0}},
    {{1, 0, 0}, {1, 0}},
    {{1, 0, 1}, {1, 1}},

    {{0, 0, 0}, {0, 0}},
    {{1, 0, 1}, {1, 1}},
    {{0, 0, 1}, {0, 1}},
};

static inline bool is_solid(BlockType t) {
  return t != BlockAir;
}

static void chunk_add_cube(Chunk *chunk, BlockType neighbours[6], size_t x, size_t y, size_t z) {
  if (!chunk || !chunk->mesh) return;

  int ccx = chunk->coords[0] * CHUNK_WIDTH;
  int ccy = chunk->coords[1] * CHUNK_HEIGHT;
  int ccz = chunk->coords[2] * CHUNK_LENGTH;

  for (size_t face = 0; face < 6; face++) {
    if (neighbours[face] != BlockAir) continue;

    Vertex face_vertices[6];
    memcpy(face_vertices, &cube[face * 6], sizeof(face_vertices));

    for (size_t i = 0; i < 6; i++) {
      face_vertices[i].pos[0] += ccx + x;
      face_vertices[i].pos[1] += ccy + y;
      face_vertices[i].pos[2] += ccz + z;
    }

    nu_mesh_add_bytes(chunk->mesh, sizeof(face_vertices), face_vertices);
  }
}

void get_neighbours(Chunk *chunk, BlockType neighbours[6], size_t x, size_t y, size_t z) {
  if (!chunk) {
    return;
  }
  neighbours[0] = (z == CHUNK_LENGTH - 1) ? BlockAir : chunk->blocks[CHUNK_INDEX(x, y, z + 1)].type;
  neighbours[1] = (z == 0) ? BlockAir : chunk->blocks[CHUNK_INDEX(x, y, z - 1)].type;
  neighbours[2] = (x == 0) ? BlockAir : chunk->blocks[CHUNK_INDEX(x - 1, y, z)].type;
  neighbours[3] = (x == CHUNK_WIDTH - 1) ? BlockAir : chunk->blocks[CHUNK_INDEX(x + 1, y, z)].type;
  neighbours[4] = (y == CHUNK_HEIGHT - 1) ? BlockAir : chunk->blocks[CHUNK_INDEX(x, y + 1, z)].type;
  neighbours[5] = (y == 0) ? BlockAir : chunk->blocks[CHUNK_INDEX(x, y - 1, z)].type;
}

void mesh_chunk(Chunk *chunk) {
  if (!chunk || !chunk->generated || !chunk->blocks) return;
  if (chunk->mesh) nu_destroy_mesh(&chunk->mesh);
  chunk->mesh = nu_create_mesh(vertex_num, vertex_sizes, vertex_counts, vertex_types);
  for (size_t x = 0; x < CHUNK_WIDTH; x++) {
    for (size_t y = 0; y < CHUNK_HEIGHT; y++) {
      for (size_t z = 0; z < CHUNK_LENGTH; z++) {
        size_t idx = CHUNK_INDEX(x, y, z);
        BlockType block = chunk->blocks[idx].type;
        if (is_solid(block)) {
          BlockType neighbours[6] = {BlockAir};
          get_neighbours(chunk, neighbours, x, y, z);
          chunk_add_cube(chunk, neighbours, x, y, z);
        }
      }
    }
  }
  nu_send_mesh(chunk->mesh);
  nu_free_mesh(chunk->mesh);
}
