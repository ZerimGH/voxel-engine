#ifndef BLOCK_H

#define BLOCK_H

typedef enum {
  BlockAir,
  BlockSolid,
} BlockType;

typedef struct {
  BlockType type;
} Block;

#endif
