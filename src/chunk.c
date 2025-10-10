#include "chunk.h"

// #define GREEDY

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

void print_chunk(Chunk *chunk) {
  if (!chunk) return (void)printf("Chunk: (null)\n");
  printf("Chunk: %p {\n", chunk);
  printf("  coords: (%d, %d, %d)\n", chunk->coords[0], chunk->coords[1], chunk->coords[2]);
  printf("  blocks: %p\n", chunk->blocks);
  printf("  generated: %d\n", chunk->generated);
  printf("  num_blocks: %zu\n", chunk->num_blocks);
  nu_print_mesh(chunk->mesh, 2);
  printf("  meshed: %d\n", chunk->meshed);
  printf("}\n");
}

void destroy_chunk(Chunk **chunk) {
  if (!chunk || !(*chunk)) return;
  if ((*chunk)->mesh) nu_destroy_mesh(&(*chunk)->mesh);
  if ((*chunk)->blocks) {
    free((*chunk)->blocks);
    (*chunk)->blocks = NULL;
  }
  **chunk = (Chunk){0};
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
    int gx = ccx + x;
    for (size_t z = 0; z < CHUNK_LENGTH; z++) {
      int gz = ccz + z;
      int terrain_height = octave_noise_2d(gx, gz, 3, 0.5, 1.6, 128, 1) * 50 + 50;
      for (size_t y = 0; y < CHUNK_HEIGHT; y++) {
        int gy = ccy + y;
        BlockType block = BlockAir;
        if (gy <= terrain_height) {
          int dist_from_surface = terrain_height - gy;
          if (dist_from_surface == 0) {
            block = BlockGrass;
          } else if (dist_from_surface <= 5) {
            block = BlockDirt;
          } else {
            block = BlockStone;
          }
        }
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
  GLint side_index;
  GLint block_type;
} Vertex;

size_t vertex_num = 4;
size_t vertex_sizes[] = {sizeof(GLfloat), sizeof(GLfloat), sizeof(GLint), sizeof(GLint)};
size_t vertex_counts[] = {3, 2, 1, 1};
GLenum vertex_types[] = {GL_FLOAT, GL_FLOAT, GL_INT, GL_INT};

#ifdef GREEDY

static const int axis_uv[3][2] = {{1, 2}, {0, 2}, {0, 1}};
static const int dims[3] = {CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_LENGTH};

// Get the rendering type of a block
// 0 = Not rendered
// 1 = Transparent
// 2 = Solid
static inline int block_render_type(BlockType t) {
  if (t == BlockAir) return 0;
  /*
  if (t == BlockWater || t == BlockGlass)
    return 1;
    */
  return 2;
}

// Write a face to a vertex buffer
static inline void emit_face(Vertex *target, size_t *count, float p[4][3], float s[4], float t[4], bool face_positive, bool flip_winding, int side_index, int block_type) {
#define EMIT(i)                                                                                                                                                                                        \
  target[(*count)++] = (Vertex) {                                                                                                                                                                      \
    {roundf(p[i][0]), roundf(p[i][1]), roundf(p[i][2])}, {s[i], t[i]}, side_index, block_type                                                                                                                                  \
  }

  static const unsigned int faces[2][6] = {
      {0, 1, 2, 2, 1, 3}, // standard winding
      {0, 2, 1, 1, 2, 3}  // flipped winding
  };

  const unsigned int *indices;
  if (flip_winding)
    indices = faces[1];
  else
    indices = faces[0];

  if (!face_positive) // invert winding for negative direction faces
    indices = (indices == faces[0]) ? faces[1] : faces[0];

  for (int i = 0; i < 6; i++) EMIT(indices[i]);

#undef EMIT
}

// Greedy meshing
void mesh_chunk(Chunk *chunk) {

  if (!chunk || !chunk->generated || !chunk->blocks || chunk->meshed) return;
  if (chunk->mesh) nu_destroy_mesh(&chunk->mesh);
  chunk->mesh = nu_create_mesh(vertex_num, vertex_sizes, vertex_counts, vertex_types);

  // Allocate array of mesh vertices
  size_t max_verts = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_LENGTH * 6;
  Vertex *verts = malloc(sizeof(Vertex) * max_verts);
  // Vertex *tverts = malloc(sizeof(Vertex) * max_verts);
  size_t vert_count = 0;
  // size_t tvert_count = 0;

  // Chunk global coordinates
  int corner_x = chunk->coords[0] * CHUNK_WIDTH;
  int corner_y = chunk->coords[1] * CHUNK_HEIGHT;
  int corner_z = chunk->coords[2] * CHUNK_LENGTH;

  // For each axis, greedily merge block faces into quads
  for (int axis = 0; axis < 3; axis++) {
    size_t u_axis = axis_uv[axis][0];
    size_t v_axis = axis_uv[axis][1];
    size_t size_u = dims[u_axis];
    size_t size_v = dims[v_axis];

    BlockType *mask = calloc(size_u * size_v, sizeof(BlockType));
    bool *face_positive_mask = calloc(size_u * size_v, sizeof(bool));

    // Loop from the -1th block ? to the maximum block in that axis
    for (int slice = -1; slice < dims[axis]; slice++) {
      // Loop over other axes
      for (size_t y = 0; y < size_v; y++) {
        for (size_t x = 0; x < size_u; x++) {
          // Current relative chunk coords
          int coords[3] = {0, 0, 0};
          coords[u_axis] = (int)x;
          coords[v_axis] = (int)y;
          coords[axis] = slice;

          // Get the current, and the next block in this axis
          BlockType blockA = (coords[axis] >= 0 && coords[axis] < dims[axis]) ? chunk->blocks[CHUNK_INDEX(coords[0], coords[1], coords[2])].type : BlockAir;

          coords[axis] = slice + 1;
          BlockType blockB = (coords[axis] >= 0 && coords[axis] < dims[axis]) ? chunk->blocks[CHUNK_INDEX(coords[0], coords[1], coords[2])].type : BlockAir;

          int ra = block_render_type(blockA);
          int rb = block_render_type(blockB);

          // If the block's rendering types dont match, there should be a quad
          // between them
          if (ra != rb) {
            mask[y * size_u + x] = (ra > rb) ? blockA : blockB;
            face_positive_mask[y * size_u + x] = (rb < ra);
          } else { // Otherwise, dont generate a quad
            mask[y * size_u + x] = BlockAir;
          }
        }
      }

      // Merge stuff
      size_t y = 0;
      while (y < size_v) {
        size_t x = 0;
        while (x < size_u) {
          BlockType current = mask[y * size_u + x];
          bool facePositive = face_positive_mask[y * size_u + x];

          if (current == BlockAir) {
            x++;
            continue;
          }

          // Merge in one direction
          size_t width = 1;
          while (x + width < size_u && mask[y * size_u + x + width] == current && face_positive_mask[y * size_u + x + width] == facePositive) {
            width++;
          }

          // Merge in the other, if all blocks match
          size_t height = 1;
          bool stop = false;
          while (y + height < size_v && !stop) {
            for (size_t k = 0; k < width; k++) {
              if (mask[(y + height) * size_u + x + k] != current || face_positive_mask[(y + height) * size_u + x + k] != facePositive) {
                stop = true;
                break;
              }
            }
            if (!stop) height++;
          }

          int base[3] = {0, 0, 0}; // Quad position
          base[u_axis] = (int)x;
          base[v_axis] = (int)y;
          base[axis] = slice + 1;

          int du[3] = {0, 0, 0}; // Quad width in U axis
          du[u_axis] = (int)width;
          int dv[3] = {0, 0, 0}; // Quad height in V axis
          dv[v_axis] = (int)height;

          // Quad corners
          float p[4][3] = {{base[0] + corner_x, base[1] + corner_y, base[2] + corner_z},
                           {base[0] + du[0] + corner_x, base[1] + du[1] + corner_y, base[2] + du[2] + corner_z},
                           {base[0] + dv[0] + corner_x, base[1] + dv[1] + corner_y, base[2] + dv[2] + corner_z},
                           {base[0] + du[0] + dv[0] + corner_x, base[1] + du[1] + dv[1] + corner_y, base[2] + du[2] + dv[2] + corner_z}};
          // Is the face pointing in the positive direction in its axis?
          bool face_positive = face_positive_mask[y * size_u + x];

          // Texcoord calculation + rotation
          float s[4], t[4];
          if (u_axis == 1 && v_axis != 1) {
            s[0] = 0;
            t[0] = 0;
            s[1] = 0;
            t[1] = (float)width;
            s[2] = (float)height;
            t[2] = 0;
            s[3] = (float)height;
            t[3] = (float)width;
          } else {
            s[0] = 0;
            t[0] = 0;
            s[1] = (float)width;
            t[1] = 0;
            s[2] = 0;
            t[2] = (float)height;
            s[3] = (float)width;
            t[3] = (float)height;
          }

          Vertex *target = verts;
          size_t *target_count = &vert_count;

          // Determine side index
          int side_index = -1;
          switch (axis) {
          case 0:
            side_index = face_positive ? 3 : 2;
            break; // X axis
          case 1:
            side_index = face_positive ? 4 : 5;
            break; // Y axis
          case 2:
            side_index = face_positive ? 0 : 1;
            break; // Z axis
          }

          // Generate vertices for mesh
          bool flip_winding = (axis == 1); // flip Y-axis to correct orientation
          emit_face(target, target_count, p, s, t, face_positive, flip_winding, side_index, current);

          // Remove quad from mask
          for (size_t i = 0; i < height; i++) {
            for (size_t j = 0; j < width; j++) {
              mask[(y + i) * size_u + x + j] = BlockAir;
              face_positive_mask[(y + i) * size_u + x + j] = false;
            }
          }

          x += width;
        }
        y++;
      }
    }

    free(mask);
    free(face_positive_mask);
  }

  // Upload vertices to meshes
  if (vert_count > 0) {
    nu_mesh_add_bytes(chunk->mesh, vert_count * sizeof(Vertex), verts);
    nu_send_mesh(chunk->mesh);
    nu_free_mesh(chunk->mesh);
  }
  // Mark as meshed
  chunk->meshed = true;
  free(verts);
}
#else

Vertex cube[] = {
    // Front face (+Z)
    {{0, 0, 1}, {0, 0}, 0, 0},
    {{1, 0, 1}, {1, 0}, 0, 0},
    {{1, 1, 1}, {1, 1}, 0, 0},
    {{0, 0, 1}, {0, 0}, 0, 0},
    {{1, 1, 1}, {1, 1}, 0, 0},
    {{0, 1, 1}, {0, 1}, 0, 0},

    // Back face (-Z)
    {{1, 0, 0}, {0, 0}, 1, 0},
    {{0, 0, 0}, {1, 0}, 1, 0},
    {{0, 1, 0}, {1, 1}, 1, 0},
    {{1, 0, 0}, {0, 0}, 1, 0},
    {{0, 1, 0}, {1, 1}, 1, 0},
    {{1, 1, 0}, {0, 1}, 1, 0},

    // Left face (-X)
    {{0, 0, 0}, {0, 0}, 2, 0},
    {{0, 0, 1}, {1, 0}, 2, 0},
    {{0, 1, 1}, {1, 1}, 2, 0},
    {{0, 0, 0}, {0, 0}, 2, 0},
    {{0, 1, 1}, {1, 1}, 2, 0},
    {{0, 1, 0}, {0, 1}, 2, 0},

    // Right face (+X)
    {{1, 0, 1}, {0, 0}, 3, 0},
    {{1, 0, 0}, {1, 0}, 3, 0},
    {{1, 1, 0}, {1, 1}, 3, 0},
    {{1, 0, 1}, {0, 0}, 3, 0},
    {{1, 1, 0}, {1, 1}, 3, 0},
    {{1, 1, 1}, {0, 1}, 3, 0},

    // Top face (+Y)
    {{0, 1, 1}, {0, 0}, 4, 0},
    {{1, 1, 1}, {1, 0}, 4, 0},
    {{1, 1, 0}, {1, 1}, 4, 0},
    {{0, 1, 1}, {0, 0}, 4, 0},
    {{1, 1, 0}, {1, 1}, 4, 0},
    {{0, 1, 0}, {0, 1}, 4, 0},

    // Bottom face (-Y)
    {{0, 0, 0}, {0, 0}, 5, 0},
    {{1, 0, 0}, {1, 0}, 5, 0},
    {{1, 0, 1}, {1, 1}, 5, 0},
    {{0, 0, 0}, {0, 0}, 5, 0},
    {{1, 0, 1}, {1, 1}, 5, 0},
    {{0, 0, 1}, {0, 1}, 5, 0},
};

static inline bool is_solid(BlockType t) {
  return t != BlockAir;
}

static void chunk_add_cube(Chunk *chunk, BlockType neighbours[6], size_t x, size_t y, size_t z, BlockType block_type) {
  if (!chunk || !chunk->mesh || block_type == BlockAir) return;

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
      face_vertices[i].block_type = block_type;
    }

    nu_mesh_add_bytes(chunk->mesh, sizeof(face_vertices), face_vertices);
  }
}

void get_neighbours(Chunk *chunk, BlockType neighbours[6], size_t x, size_t y, size_t z) {
  if (!chunk) return;

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
          chunk_add_cube(chunk, neighbours, x, y, z, block);
        }
      }
    }
  }

  nu_send_mesh(chunk->mesh);
  nu_free_mesh(chunk->mesh);
}

#endif
