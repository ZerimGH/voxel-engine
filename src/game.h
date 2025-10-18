#ifndef GAME_H

#define GAME_H

#include "camera.h"
#include "clouds.h"
#include "crosshair.h"
#include "defines.h"
#include "nuGL.h"
#include "player.h"
#include "sky.h"
#include "ui_renderer.h"
#include "world.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

typedef struct {
  nu_Window *window;
  SkyRenderer *sky_renderer;
  Player *player;
  World *world;
  UiRenderer *ui_renderer;
  Crosshair *crosshair;
  Clouds *clouds;
  float this_time, last_time;
} Game;

Game *create_game(void);
void destroy_game(Game **game);
void update_game(Game *game);
void render_game(Game *game);
bool game_over(Game *game);
#endif
