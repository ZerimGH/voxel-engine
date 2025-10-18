#include "game.h"

#define WIN_WIDTH 1920 
#define WIN_HEIGHT 1080 

#define FULLSCREEN true

Game *create_game(void) {
  char err_msg[1024] = {0};
  nu_Window *window = NULL;
  SkyRenderer *sky_renderer = NULL;
  Player *player = NULL;
  World *world = NULL;
  Game *game = NULL;
  UiRenderer *ui_renderer = NULL;
  Crosshair *crosshair = NULL;
  Clouds *clouds = NULL;

  // Create window
  window = nu_create_window(WIN_WIDTH, WIN_HEIGHT, NULL, FULLSCREEN);
  if (!window) {
    sprintf(err_msg, "(create_game): Error creating game: nu_create_window() returned NULL.\n");
    goto failure;
  }

  glfwSetInputMode(window->glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Create sky renderer
  sky_renderer = create_sky_renderer();
  if (!sky_renderer) {
    sprintf(err_msg, "(create_game): Error creating game: create_sky_renderer() returned NULL\n");
    goto failure;
  }

  // Create camera
  player = create_player();
  if (!player) {
    sprintf(err_msg, "(create_game): Error creating game: create_player() returned NULL\n");
    goto failure;
  }

  // Create world
  world = create_world();
  if (!world) {
    sprintf(err_msg, "(create_game): Error creating game: create_world() returned NULL\n");
    goto failure;
  }

  // Create ui_renderer
  ui_renderer = create_ui_renderer();
  if (!ui_renderer) {
    sprintf(err_msg, "(create_game): Error creating game: create_ui_renderer() returned NULL\n");
    goto failure;
  }

  // Create crosshair
  crosshair = create_crosshair();
  if (!ui_renderer) {
    sprintf(err_msg, "(create_game): Error creating game: create_crosshair() returned NULL\n");
    goto failure;
  }

  clouds = create_clouds();
  if(!clouds) {
    sprintf(err_msg, "(create_game): Error creating game: create_clouds() returned NULL\n");
    goto failure;
  }

  // Create game
  game = calloc(1, sizeof(Game));
  if (!game) {
    sprintf(err_msg, "(create_game): Error creating game: calloc failed for Game *game.\n");
    goto failure;
  }
  
  goto success;

failure:
  fprintf(stderr, "%.1023s", err_msg);
  nu_destroy_window(&window);
  destroy_sky_renderer(&sky_renderer);
  destroy_player(&player);
  destroy_world(&world);
  destroy_ui_renderer(&ui_renderer);
  destroy_crosshair(&crosshair);
  destroy_clouds(&clouds);
  if (game) free(game);
  return NULL;

success:
  // Set things
  game->window = window;
  game->sky_renderer = sky_renderer;
  game->player = player;
  game->world = world;
  game->ui_renderer = ui_renderer;
  game->crosshair = crosshair;
  game->clouds = clouds;
  game->last_time = 0.f;
  game->this_time = glfwGetTime();
  return game;
}

void destroy_game(Game **game) {
  if (!game || !(*game)) return;
  nu_destroy_window(&(*game)->window);
  destroy_sky_renderer(&(*game)->sky_renderer);
  destroy_ui_renderer(&(*game)->ui_renderer);
  destroy_crosshair(&(*game)->crosshair);
  destroy_player(&(*game)->player);
  destroy_clouds(&(*game)->clouds);
  destroy_world(&(*game)->world);
  free(*game);
  *game = NULL;
}

void update_game(Game *game) {
  if (!game) return;
  // Get the time of this frame
  game->this_time = glfwGetTime();

  // Update player based on input
  game->player->dt = game->this_time - game->last_time;
  if (nu_get_key_state(game->window, GLFW_KEY_S)) player_backwards(game->player);
  if (nu_get_key_state(game->window, GLFW_KEY_W)) player_forwards(game->player);
  if (nu_get_key_state(game->window, GLFW_KEY_A)) player_left(game->player);
  if (nu_get_key_state(game->window, GLFW_KEY_D)) player_right(game->player);
  player_set_jumping(game->player, nu_get_key_state(game->window, GLFW_KEY_SPACE));
  player_set_crouching(game->player, nu_get_key_state(game->window, GLFW_KEY_LEFT_SHIFT));
  player_set_sprinting(game->player, nu_get_key_pressed(game->window, GLFW_KEY_LEFT_CONTROL) || (nu_get_key_pressed(game->window, GLFW_KEY_W) && nu_get_key_state(game->window, GLFW_KEY_LEFT_CONTROL)));
  player_rotate(game->player, nu_get_delta_mouse_x(game->window), -nu_get_delta_mouse_y(game->window));
  player_update(game->player, game->world);
  if (game->window->mouse_left && !game->window->last_mouse_left) player_break(game->player, game->world);
  if (game->window->mouse_right && !game->window->last_mouse_right) player_place(game->player, game->world);
  
  // Update world so chunks load around player 
  int nx = (int)(floorf(game->player->position[0] / CHUNK_WIDTH));
  int ny = (int)(floorf(game->player->position[1] / CHUNK_HEIGHT));
  int nz = (int)(floorf(game->player->position[2] / CHUNK_LENGTH));
  world_update_centre(game->world, nx, ny, nz);

  // If not multithreaded, update the world from this thread
#ifndef MULTITHREAD
  float time_budget = 1.f / 120.f;
  float start_time = glfwGetTime();
  do {
    if (!world_update_queue(game->world)) break;
  } while (glfwGetTime() - start_time < time_budget);
#endif

  // Set time
  game->last_time = game->this_time;

  nu_update_input(game->window);
}

void render_game(Game *game) {
  if (!game) return;
  // Calculate camera's vp
  mat4 vp = {0};
  float aspect = (float)game->window->width / (float)game->window->height;
  camera_calculate_vp_matrix(game->player->camera, vp, aspect);

  // Render everything
  nu_start_frame(game->window);

  render_sky(game->sky_renderer, (float)game->window->height, game->player->camera->pitch, game->player->camera->fov);
  render_world(game->world, vp);
  render_clouds(game->clouds, game->player, game->this_time, aspect); 
  render_crosshair(game->crosshair, game->ui_renderer, (float)game->window->width, (float)game->window->height);

  nu_end_frame(game->window);
}

bool game_over(Game *game) {
  if (!game) return true;
  return glfwWindowShouldClose(game->window->glfw_window);
}
