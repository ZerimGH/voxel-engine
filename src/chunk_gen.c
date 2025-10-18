#include "noise.h"

void generate_chunk(Chunk *chunk, uint32_t seed) {
  if (!chunk)
    return;
  ChunkState state = chunk->state;
  if (state != STATE_EMPTY) {
    return;
  }
  if (chunk->blocks)
    free(chunk->blocks);
  chunk->blocks = calloc(CHUNK_VOLUME, sizeof(Block));
  if (!chunk->blocks) {
    fprintf(stderr,
            "(generate_chunk): Couldn't generate chunk at coords (%d, %d, %d), "
            "calloc failed.\n",
            chunk->coords[0], chunk->coords[1], chunk->coords[2]);
    return;
  }

  int ccx = chunk->coords[0] * CHUNK_WIDTH;
  int ccy = chunk->coords[1] * CHUNK_HEIGHT;
  int ccz = chunk->coords[2] * CHUNK_LENGTH;
  for (size_t x = 0; x < CHUNK_WIDTH; x++) {
    int gx = ccx + x;
    for (size_t z = 0; z < CHUNK_LENGTH; z++) {
      int gz = ccz + z;
      float noise_val = octave_noise_2d(gx, gz, 4, 0.35, 1.6, 128, seed);
      float sand_val = octave_noise_2d(gx, gz, 4, 0.35, 1.6, 128, seed + 1);
      int terrain_height = noise_val * 50 + 50;
      for (size_t y = 0; y < CHUNK_HEIGHT; y++) {
        int gy = ccy + y;
        BlockType block = BlockAir;
        if (gy <= terrain_height) {
          int dist_from_surface = terrain_height - gy;
          if (dist_from_surface == 0) {
            block = sand_val > 0 ? BlockGrass : BlockSand;
          } else if (dist_from_surface <= 5) {
            block = sand_val > 0 ? BlockDirt : BlockSand;
          } else {
            block = BlockStone;
          }
        }
        size_t idx = CHUNK_INDEX(x, y, z);
        chunk->blocks[idx] = (Block){.type = block};
      }
    }
  }

  chunk->state = STATE_NEEDS_MESH;
}
