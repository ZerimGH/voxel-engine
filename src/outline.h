#ifndef OUTLINE_H
#define OUTLINE_H

// Includes
#include <stdio.h>
#include <stdlib.h>
#include "nuGL.h"
#include "world.h"
#include "player.h"
#include "camera.h"

// Structs
typedef struct {
  nu_Program *program;
  nu_Mesh *mesh;
} OutlineRenderer;

// Function prototypes
OutlineRenderer *create_outline_renderer(void);
void destroy_outline_renderer(OutlineRenderer **outline);
void render_outline(OutlineRenderer *outline, Player *player, float width, float height);

#endif // outline.h
