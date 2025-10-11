#ifndef GAME_H

#define GAME_H

#include "camera.h"
#include "defines.h"
#include "nuGL.h"
#include "sky.h"
#include "world.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  nu_Window *window;
  SkyRenderer *sky_renderer;
  Camera *camera;
  World *world;
} Game;

Game *create_game(void);
void destroy_game(Game **game);
void update_game(Game *game);
void render_game(Game *game);
bool game_over(Game *game);
#endif
