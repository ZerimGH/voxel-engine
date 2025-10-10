#ifndef BLOCK_H

#define BLOCK_H

#define NUM_BLOCKS 3

#define BLOCK_TEXTURES                                                                                                                                                                                 \
  "textures/dirt-top.png", "textures/dirt-side.png", "textures/dirt-bottom.png", "textures/grass-top.png", "textures/grass-side.png", "textures/grass-bottom.png", "textures/stone-top.png",           \
      "textures/stone-side.png", "textures/stone-bottom.png"

#define NUM_BLOCK_TEXTURES (NUM_BLOCKS * 3)

typedef enum {
  BlockAir,
  BlockDirt,
  BlockGrass,
  BlockStone,
} BlockType;

typedef struct {
  BlockType type;
} Block;

#endif
