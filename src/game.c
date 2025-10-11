#include "game.h"

Game *create_game(void) {
  char err_msg[1024] = {0};
  nu_Window *window = NULL;
  SkyRenderer *sky_renderer = NULL;
  Camera *camera = NULL;
  World *world = NULL;
  Game *game = NULL;

  // Create window
  window = nu_create_window(600, 600, NULL, NULL);
  if (!window) {
    sprintf(err_msg, "(create_game): Error creating game: nu_create_window() returned NULL.\n");
    goto failure;
  }

  glfwSetInputMode(window->glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // Create sky renderer
  sky_renderer = create_sky_renderer();
  if (!sky_renderer) {
    sprintf(err_msg, "(create_game): Error creating game: create_sky_renderer() returned NULL\n");
    goto failure;
  }

  // Create camera
  camera = create_camera(0.5, 512, 90.f);
  if (!camera) {
    sprintf(err_msg, "(create_game): Error creating game: create_camera() returned NULL\n");
    goto failure;
  }

  // Create world
  world = create_world();
  if (!world) {
    sprintf(err_msg, "(create_game): Error creating game: create_world() returned NULL\n");
    goto failure;
  }

  // Create game
  game = calloc(1, sizeof(Game));
  if (!game) {
    sprintf(err_msg, "(create_game): Error creating game: calloc failed for Game *game.\n");
    goto failure;
  }

  // Set things
  game->window = window;
  game->sky_renderer = sky_renderer;
  game->camera = camera;
  game->world = world;
  goto success;

failure:
  fprintf(stderr, "%1024s", err_msg);
  nu_destroy_window(&window);
  destroy_sky_renderer(&sky_renderer);
  destroy_camera(&camera);
  destroy_world(&world);
  if (game) free(game);
  return NULL;

success:
  return game;
}

void destroy_game(Game **game) {
  if (!game || !(*game)) return;
  nu_destroy_window(&(*game)->window);
  destroy_sky_renderer(&(*game)->sky_renderer);
  destroy_camera(&(*game)->camera);
  destroy_world(&(*game)->world);
  free(*game);
}

void update_game(Game *game) {
  if (!game) return;
#define MOVE_SPEED 3.f
#define SENS 0.0015f
  // Poll events, move camera
  if (nu_get_key_state(game->window, GLFW_KEY_S)) camera_move(game->camera, (vec3){0, 0, -MOVE_SPEED});
  if (nu_get_key_state(game->window, GLFW_KEY_W)) camera_move(game->camera, (vec3){0, 0, MOVE_SPEED});
  if (nu_get_key_state(game->window, GLFW_KEY_A)) camera_move(game->camera, (vec3){-MOVE_SPEED, 0, 0});
  if (nu_get_key_state(game->window, GLFW_KEY_D)) camera_move(game->camera, (vec3){MOVE_SPEED, 0, 0});
  if (nu_get_key_state(game->window, GLFW_KEY_SPACE)) camera_move(game->camera, (vec3){0, MOVE_SPEED, 0});
  if (nu_get_key_state(game->window, GLFW_KEY_LEFT_SHIFT)) camera_move(game->camera, (vec3){0, -MOVE_SPEED, 0});
  camera_rotate(game->camera, nu_get_delta_mouse_x(game->window) * SENS, -nu_get_delta_mouse_y(game->window) * SENS);
  nu_update_input(game->window);
#undef MOVE_SPEED
#undef SENS
  // Update world so chunks load around camera
  int nx = (int)(floorf(game->camera->position[0] / CHUNK_WIDTH));
  int ny = (int)(floorf(game->camera->position[1] / CHUNK_HEIGHT));
  int nz = (int)(floorf(game->camera->position[2] / CHUNK_LENGTH));
  world_update_centre(game->world, nx, ny, nz);
  float time_budget = 1.f/120.f;
  float start_time = glfwGetTime();
  do {
    if(!world_update_queue(game->world)) break;
  } while (glfwGetTime() - start_time < time_budget);
}

void render_game(Game *game) {
  if (!game) return;
  // Calculate vp
  mat4 vp = {0};
  float aspect = (float)game->window->width / (float)game->window->height;
  camera_calculate_vp_matrix(game->camera, vp, aspect);
  // Render everything
  nu_start_frame(game->window);
  render_sky(game->sky_renderer, (float)game->window->height, game->camera->pitch, game->camera->fov);
  render_world(game->world, vp);
  // Render the world
  nu_end_frame(game->window);
}

bool game_over(Game *game) {
  if (!game) return true;
  return glfwWindowShouldClose(game->window->glfw_window);
}
