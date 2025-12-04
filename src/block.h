#ifndef BLOCK_h

#define BLOCK_h

#define NUM_BLOCKS 6

#define NUM_BLOCK_TEXTURES (NUM_BLOCKS * 3)

#define BLOCK_TEXTURES \
"textures/Dirt-top.png", "textures/Dirt-side.png", "textures/Dirt-bottom.png", \
"textures/Grass-top.png", "textures/Grass-side.png", "textures/Grass-bottom.png", \
"textures/Stone-top.png", "textures/Stone-side.png", "textures/Stone-bottom.png", \
"textures/Sand-top.png", "textures/Sand-side.png", "textures/Sand-bottom.png", \
"textures/Log-top.png", "textures/Log-side.png", "textures/Log-bottom.png", \
"textures/DiamondOre-top.png", "textures/DiamondOre-side.png", "textures/DiamondOre-bottom.png"

#include <stdint.h>

typedef uint8_t BlockType;

enum {
  BlockAir,
  BlockDirt,
  BlockGrass,
  BlockStone,
  BlockSand,
  BlockLog,
  BlockDiamondOre,

};

typedef struct {
  BlockType type;
} Block;

#endif