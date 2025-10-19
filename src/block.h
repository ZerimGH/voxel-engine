#ifndef BLOCK_h

#define BLOCK_h

#define NUM_BLOCKS 6

#define NUM_BLOCK_TEXTURES (NUM_BLOCKS * 3)

#define BLOCK_TEXTURES                                                         \
  "textures/dirt-top.png", "textures/dirt-side.png",                           \
      "textures/dirt-bottom.png", "textures/grass-top.png",                    \
      "textures/grass-side.png", "textures/grass-bottom.png",                  \
      "textures/stone-top.png", "textures/stone-side.png",                     \
      "textures/stone-bottom.png", "textures/sand-top.png",                    \
      "textures/sand-side.png", "textures/sand-bottom.png",                    \
      "textures/log-top.png", "textures/log-side.png",                         \
      "textures/log-bottom.png", "textures/diamond_ore-top.png",               \
      "textures/diamond_ore-side.png", "textures/diamond_ore-bottom.png"

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