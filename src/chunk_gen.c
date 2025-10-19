#define FNL_IMPL
#include "FastNoiseLite.h"

void generate_chunk(Chunk *chunk, uint32_t seed) {
  static fnl_state noise;
  static int noise_init = 0;

  if(!noise_init) {
    noise_init = 1;
    noise = fnlCreateState();

    noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noise.frequency = 0.005f;
    noise.fractal_type = FNL_FRACTAL_FBM;
    noise.octaves = 3;
    noise.lacunarity = 2.3f;
    noise.gain = 0.4f;
  }

  noise.seed = seed;

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

  static float heightmap[CHUNK_AREA];
  static float sandmap[CHUNK_AREA];

  int ccx = chunk->coords[0] * CHUNK_WIDTH;
  int ccy = chunk->coords[1] * CHUNK_HEIGHT;
  int ccz = chunk->coords[2] * CHUNK_LENGTH;

  for(size_t x = 0; x < CHUNK_WIDTH; x++) {
    int gx = ccx + x;
    for(size_t z = 0; z < CHUNK_LENGTH; z++) {
      int gz = ccz + z;
      float height_val = fnlGetNoise2D(&noise, gx, gz);
      height_val = height_val / 2.f + 0.5f;
      height_val = height_val * 50;
      heightmap[CHUNK_INDEX(x, 0, z)] = height_val;
      float sand_val = fnlGetNoise2D(&noise, gx + 1000, gz + 1000);
      sandmap[CHUNK_INDEX(x, 0, z)] = sand_val;
    }
  }

  for (size_t x = 0; x < CHUNK_WIDTH; x++) {
    for (size_t z = 0; z < CHUNK_LENGTH; z++) {
      float height_val = heightmap[CHUNK_INDEX(x, 0, z)];
      float sand_val = sandmap[CHUNK_INDEX(x, 0, z)];
      if(ccy > height_val) continue;
      for (size_t y = 0; y < CHUNK_HEIGHT; y++) {
        int gy = ccy + y;
        BlockType block = BlockAir;
        if (gy <= height_val) {
          int dist_from_surface = height_val - gy;
          if (dist_from_surface == 0) {
            block = sand_val < 0 ? BlockSand : BlockGrass;
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

  chunk->state = STATE_NEEDS_MESH;
}
