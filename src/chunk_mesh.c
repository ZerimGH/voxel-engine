static const int axis_uv[3][2] = {{1, 2}, {0, 2}, {0, 1}};
static const int dims[3] = {CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_LENGTH};

// Get the rendering type of a block
// 0 = Not rendered
// 1 = Transparent
// 2 = Solid
static inline int block_render_type(BlockType t) {
  if (t == BlockAir)
    return 0;
  /*
  if (t == BlockWater || t == BlockGlass)
    return 1;
    */
  return 2;
}

// Write a face to a vertex buffer
static inline void emit_face(Vertex *target, size_t *count, float p[4][3],
                             float s[4], float t[4], bool face_positive,
                             bool flip_winding, int side_index,
                             int block_type) {
#define EMIT(i)                                                                \
  target[(*count)++] = (Vertex) {                                              \
    {roundf(p[i][0]), roundf(p[i][1]), roundf(p[i][2])}, {s[i], t[i]},         \
        side_index, block_type                                                 \
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

  for (int i = 0; i < 6; i++)
    EMIT(indices[i]);

#undef EMIT
}

// Greedy meshing
void mesh_chunk(Chunk *chunk) {

  if (!chunk)
    return;
  if (!chunk->blocks)
    return;

  ChunkState state = chunk->state;
  if (state != STATE_NEEDS_MESH) {
    return;
  }

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
          BlockType blockA =
              (coords[axis] >= 0 && coords[axis] < dims[axis])
                  ? chunk->blocks[CHUNK_INDEX(coords[0], coords[1], coords[2])]
                        .type
                  : BlockAir;

          coords[axis] = slice + 1;
          BlockType blockB =
              (coords[axis] >= 0 && coords[axis] < dims[axis])
                  ? chunk->blocks[CHUNK_INDEX(coords[0], coords[1], coords[2])]
                        .type
                  : BlockAir;

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
          while (x + width < size_u &&
                 mask[y * size_u + x + width] == current &&
                 face_positive_mask[y * size_u + x + width] == facePositive) {
            width++;
          }

          // Merge in the other, if all blocks match
          size_t height = 1;
          bool stop = false;
          while (y + height < size_v && !stop) {
            for (size_t k = 0; k < width; k++) {
              if (mask[(y + height) * size_u + x + k] != current ||
                  face_positive_mask[(y + height) * size_u + x + k] !=
                      facePositive) {
                stop = true;
                break;
              }
            }
            if (!stop)
              height++;
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
          float p[4][3] = {
              {base[0] + corner_x, base[1] + corner_y, base[2] + corner_z},
              {base[0] + du[0] + corner_x, base[1] + du[1] + corner_y,
               base[2] + du[2] + corner_z},
              {base[0] + dv[0] + corner_x, base[1] + dv[1] + corner_y,
               base[2] + dv[2] + corner_z},
              {base[0] + du[0] + dv[0] + corner_x,
               base[1] + du[1] + dv[1] + corner_y,
               base[2] + du[2] + dv[2] + corner_z}};
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

          // Fix texcoords for some faces
          if (!(((u_axis != 1 && face_positive) || (v_axis != 1 && !face_positive)))) {
            for (int i = 0; i < 4; i++) {
              s[i] = (float)width - s[i];
            }
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
          emit_face(target, target_count, p, s, t, face_positive, flip_winding,
                    side_index, current);

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
  }
  free(verts);

  chunk->state = STATE_NEEDS_SEND;
}
