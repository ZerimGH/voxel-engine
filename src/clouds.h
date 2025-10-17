#ifndef CLOUDS_H

#define CLOUDS_H

#include "nuGL.h"
#include "player.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
  nu_Program *program;
  nu_Texture *texture;
  nu_Mesh *mesh;
} Clouds;

Clouds *create_clouds(void);
void destroy_clouds(Clouds **clouds);
void render_clouds(Clouds *clouds, Player *player, float time); 

#endif
