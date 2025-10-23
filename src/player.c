#include "player.h"
#include <math.h>

#define GRAVITY -31.36f                   // blocks/s ^ 2
#define PLAYER_MOVE_ACCELERATION 55.f     // blocks/s ^ 2
#define PLAYER_MOVE_SPEED 4.317f          // max blocks/s
#define PLAYER_SPRINT_MULT 1.3f           // multiplier to speed when sprinting
#define PLAYER_CROUCH_MULT 0.3f           // multiplier to speed when crouching
#define PLAYER_JUMP_HEIGHT 1.252f         // num blocks
#define PLAYER_EYE_HEIGHT 1.62f           // camera height standing
#define PLAYER_EYE_HEIGHT_CROUCHING 1.27f // camera height crouching
#define HITBOX_WIDTH 0.6f
#define HITBOX_HEIGHT 1.8f
#define HITBOX_LENGTH 0.6f

#define SENSITIVITY 0.0025f
#define WALKING_FOV 90.f
#define SPRINTING_FOV 110.f

#define AIR_FRICTION 0.78f
#define GROUND_FRICTION 0.75f

// Create a player
Player *create_player(void) {
  Player *player = calloc(1, sizeof(Player));
  if (!player) {
    fprintf(stderr,
            "(create_player): Error initialising player: calloc failed.\n");
    return NULL;
  }
  player->camera = create_camera(0.1, 512, WALKING_FOV);
  if (!player->camera) {
    fprintf(stderr, "(create_player): Error initialising player: "
                    "create_camera() returned NULL/\n");
    free(player);
    return NULL;
  }

  // Set hitbox size
  player->hitbox_dims[0] = HITBOX_WIDTH;
  player->hitbox_dims[1] = HITBOX_HEIGHT;
  player->hitbox_dims[2] = HITBOX_LENGTH;

  // Set position to (0, 100, 0)
  glm_vec3_zero(player->position);
  player->position[1] = 100;

  // No velocity
  glm_vec3_zero(player->velocity);

  // Zero other variables
  glm_vec3_zero(player->movement);
  player->grounded = false;
  player->last_grounded = false;
  player->is_crouching = false;
  player->is_sprinting = false;
  player->is_zooming = false;
  player->is_jumping = false;
  player->keep_sprint = false;
  player->jump_time = 0.f;
  player->dt = 0.f;
  return player;
}

void destroy_player(Player **player) {
  if (!player || !*player)
    return;
  if ((*player)->camera)
    destroy_camera(&(*player)->camera);
  free(*player);
  *player = NULL;
}

// Rotate a player based on mouse movement
void player_rotate(Player *player, double dx, double dy) {
  // Scale by sensitivity
  dx *= SENSITIVITY;
  dy *= SENSITIVITY;
  // Rotate player's camera
  camera_rotate(player->camera, dx, dy);
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Match the players camera position to its position
static void update_camera_pos(Player *player) {
  player->camera->position[0] = player->position[0];
  player->camera->position[1] =
      player->position[1] - player->hitbox_dims[1] / 2.f + PLAYER_EYE_HEIGHT -
      (PLAYER_EYE_HEIGHT - PLAYER_EYE_HEIGHT_CROUCHING) * player->crouch_interp;
  player->camera->position[2] = player->position[2];
  player->camera->fov = WALKING_FOV;
  player->camera->fov = WALKING_FOV + (SPRINTING_FOV - WALKING_FOV) * player->sprint_interp;
  player->camera->fov /= (player->zoom_interp * 3.f) + 1.f;
}

// Move the player forwards
void player_forwards(Player *player) {
  vec3 dir;
  camera_calculate_forward(player->camera, dir);
  dir[1] = 0.f;
  glm_vec3_normalize(dir);
  glm_vec3_add(player->movement, dir, player->movement);
  player->keep_sprint = true;
}

// Backwards...
void player_backwards(Player *player) {
  vec3 dir;
  camera_calculate_forward(player->camera, dir);
  dir[1] = 0.f;
  glm_vec3_normalize(dir);
  glm_vec3_sub(player->movement, dir, player->movement);
}

void player_right(Player *player) {
  vec3 dir;
  camera_calculate_right(player->camera, dir);
  dir[1] = 0.f;
  glm_vec3_normalize(dir);
  glm_vec3_add(player->movement, dir, player->movement);
}

void player_left(Player *player) {
  vec3 dir;
  camera_calculate_right(player->camera, dir);
  dir[1] = 0.f;
  glm_vec3_normalize(dir);
  glm_vec3_sub(player->movement, dir, player->movement);
}

void player_set_sprinting(Player *player, bool state) {
  if (!player)
    return;
  if (player->is_crouching)
    return;
  if (state)
    player->is_sprinting = true;
  // Dont modify is_sprinting if state false, so the player can keep sprinting
}

void player_set_crouching(Player *player, bool state) {
  if (!player)
    return;
  player->is_crouching = state;
  if (state)
    player->is_sprinting = false; // Can't sprint and crouch
}

void player_set_zooming(Player *player, bool state) {
  if (!player)
    return;
  player->is_zooming = state;
}

void player_set_jumping(Player *player, bool state) {
  player->is_jumping = state;
}

// Check if a position is within a hitbox
static bool pos_collides_specific(vec3 pos, vec3 hitbox_dims, float bx,
                                  float by, float bz) {
  float hx = hitbox_dims[0] * 0.5f;
  float hy = hitbox_dims[1] * 0.5f;
  float hz = hitbox_dims[2] * 0.5f;

  float min_x = pos[0] - hx;
  float max_x = pos[0] + hx;
  float min_y = pos[1] - hy;
  float max_y = pos[1] + hy;
  float min_z = pos[2] - hz;
  float max_z = pos[2] + hz;

  int ix = (int)floorf(bx);
  int iy = (int)floorf(by);
  int iz = (int)floorf(bz);

  float bmin_x = (float)ix;
  float bmax_x = bmin_x + 1.0f;
  float bmin_y = (float)iy;
  float bmax_y = bmin_y + 1.0f;
  float bmin_z = (float)iz;
  float bmax_z = bmin_z + 1.0f;

  return (min_x < bmax_x && max_x > bmin_x) &&
         (min_y < bmax_y && max_y > bmin_y) &&
         (min_z < bmax_z && max_z > bmin_z);
}

// Check if a position collides with a solid block in the world
static bool pos_collides(vec3 pos, vec3 hitbox_dims, World *world) {
  int xa = (int)floorf(pos[0] - hitbox_dims[0] / 2.f);
  int ya = (int)floorf(pos[1] - hitbox_dims[1] / 2.f);
  int za = (int)floorf(pos[2] - hitbox_dims[2] / 2.f);
  int xb = (int)floorf(pos[0] + hitbox_dims[0] / 2.f);
  int yb = (int)floorf(pos[1] + hitbox_dims[1] / 2.f);
  int zb = (int)floorf(pos[2] + hitbox_dims[2] / 2.f);

  for (int x = xa; x <= xb; x++) {
    for (int y = ya; y <= yb; y++) {
      for (int z = za; z <= zb; z++) {
        Block *block = world_get_block(world, x, y, z);
        if (!block || block->type == BlockAir)
          continue;
        if (pos_collides_specific(pos, hitbox_dims, (float)x, (float)y,
                                  (float)z)) {
          return true;
        }
      }
    }
  }
  return false;
}

// Clamp movement speed based on sprinting / crouching
static void clamp_horizontal_velocity(Player *player) {
  vec3 vel_horizontal = {player->velocity[0], 0.f, player->velocity[2]};
  float speed = glm_vec3_norm(vel_horizontal);

  float max_speed = PLAYER_MOVE_SPEED;
  if (player->is_sprinting)
    max_speed *= PLAYER_SPRINT_MULT;
  if (player->is_crouching)
    max_speed *= PLAYER_CROUCH_MULT;

  if (speed > max_speed) {
    glm_vec3_scale(vel_horizontal, max_speed / speed, vel_horizontal);
    player->velocity[0] = vel_horizontal[0];
    player->velocity[2] = vel_horizontal[2];
  }
}

// Apply acceleration, scaled by deltatime
static void player_accelerate(Player *player, vec3 acceleration) {
  vec3 scaled_accel;
  glm_vec3_scale(acceleration, player->dt, scaled_accel);
  glm_vec3_add(player->velocity, scaled_accel, player->velocity);
  clamp_horizontal_velocity(player);
}

// Normalise the players movement direction if they aren't crouching
static void player_normalise_movement(Player *player) {
  if (glm_vec3_norm(player->movement) > 0.001f) {
    vec3 dir;
    // Normalise movement if not crouching
    if (player->is_crouching) {
      glm_vec3_copy(player->movement, dir);
    } else {
      glm_vec3_normalize_to(player->movement, dir);
    }
  }
}

// Accelerate the player based on movement direction
static void player_apply_horizontal_movement(Player *player) {
  float accel = PLAYER_MOVE_ACCELERATION;
  // Normalise movement direction (if not crouching)
  player_normalise_movement(player);
  // Scale based on sprinting / crouching
  if (player->is_sprinting)
    accel *= PLAYER_SPRINT_MULT;
  if (player->is_crouching)
    accel *= PLAYER_CROUCH_MULT;
  glm_vec3_scale(player->movement, accel, player->movement);
  player_accelerate(player, player->movement);
}

// Apply players velocity, with collisions
static void player_apply_vel(Player *player, World *world) {
  vec3 new_pos;
  glm_vec3_copy(player->position, new_pos);

  for (int i = 0; i < 3; i++) {
    float remaining = player->velocity[i] * player->dt;
    const float max_step = 0.05f;
    float step = (remaining > 0) ? max_step : -max_step;

    while (fabsf(remaining) > 0.f) {
      float move = (fabsf(remaining) < fabsf(step)) ? remaining : step;
      vec3 test_pos;
      glm_vec3_copy(new_pos, test_pos);
      test_pos[i] += move;

      if (!pos_collides(test_pos, player->hitbox_dims, world)) {
        if (player->grounded && player->is_crouching && i != 1) {
          vec3 test_pos_minus;
          glm_vec3_copy(test_pos, test_pos_minus);
          test_pos_minus[1] -= 0.01f;
          if (!pos_collides(test_pos_minus, player->hitbox_dims, world)) {
            break;
          } else {
            new_pos[i] = test_pos[i];
            remaining -= move;
          }
        } else {
          new_pos[i] = test_pos[i];
          remaining -= move;
        }
      } else {
        player->velocity[i] = 0.f;
        if (i != 1)
          player->keep_sprint = false;
        break;
      }
    }
  }

  glm_vec3_copy(new_pos, player->position);
}

// Apply gravity to player
static void player_apply_gravity(Player *player) {
  player->velocity[1] += GRAVITY * player->dt;
}

// Apply friction to player, with less friction when in the air
static void player_apply_friction(Player *player) {
  float factor = player->grounded ? GROUND_FRICTION : AIR_FRICTION;
  float decay = powf(factor, player->dt * 60.0f);
  player->velocity[0] *= decay;
  player->velocity[2] *= decay;
}

static void player_handle_jump(Player *player) {
  player->jump_time -= player->dt;
  if (player->jump_time < 0.f)
    player->jump_time = 0.f;
  if (player->is_jumping) {
    if (player->grounded && player->jump_time == 0.f) {
      player->velocity[1] = sqrtf(2.f * fabsf(GRAVITY) * PLAYER_JUMP_HEIGHT);
      player->jump_time = 0.55f; // 0.55 seconds between jumps while holding
    }
  } else
    player->jump_time = 0.f; // Reset timer if not jumping
}

static void player_check_grounded(Player *player, World *world) {
  vec3 pos_under;
  glm_vec3_copy(player->position, pos_under);
  pos_under[1] -= 0.01f;
  player->grounded = pos_collides(pos_under, player->hitbox_dims, world);
}

static RayCastReturn player_eye_raycast(Player *player, World *world);

// Update a player
void player_update(Player *player, World *world) {
  // Check if grounded
  player_check_grounded(player, world);

  // If the player is jumping, jump
  player_handle_jump(player);

  // Movement normalisation
  player_apply_horizontal_movement(player);

  // Apply forces
  player_apply_gravity(player);
  player_apply_vel(player, world);
  player_apply_friction(player);

  // Set camera position
  update_camera_pos(player);

  // Crouch / sprint animation times
  if (player->is_crouching) {
    player->crouch_interp += player->dt * 10.f;
    player->crouch_interp = MIN(player->crouch_interp, 1.f);
  } else {
    player->crouch_interp -= player->dt * 10.f;
    player->crouch_interp = MAX(player->crouch_interp, 0.f);
  }
  if (player->is_sprinting) {
    if(player->keep_sprint) {
      player->sprint_interp += player->dt * 7.5f;
      player->sprint_interp = MIN(player->sprint_interp, 1.f);
    }
  } else {
    player->sprint_interp -= player->dt * 7.5f;
    player->sprint_interp = MAX(player->sprint_interp, 0.f);
  }
  if (player->is_zooming) {
    player->zoom_interp += player->dt * 2.5f;
    player->zoom_interp = MIN(player->zoom_interp, 1.f);
  } else {
    player->zoom_interp -= player->dt * 4.5f;
    player->zoom_interp = MAX(player->zoom_interp, 0.f);
  }

  // If the player shouldn't keep sprinting, unsprint
  if (!player->keep_sprint)
    player->is_sprinting = false;
  player->keep_sprint = false;

  player->last_grounded = player->grounded;
  // Set movement to 0 for next frame
  glm_vec3_zero(player->movement);
  // Update selection
  player->selection = player_eye_raycast(player, world);
}

static RayCastReturn player_eye_raycast(Player *player, World *world) {
  float x, y, z;
  x = player->camera->position[0];
  y = player->camera->position[1] + 0.01;
  z = player->camera->position[2];
  vec3 dir = {0};
  camera_calculate_forward(player->camera, dir);
  return world_raycast(world, x, y, z, dir[0], dir[1], dir[2], 4.5f);
}

// Break a block
bool player_break(Player *player, World *world) {
  if(!player || !world) return false;
  if(!player->selection.hit || !player->selection.block_hit) return false;
  world_set_block(world, BlockAir, player->selection.hit_x, player->selection.hit_y, player->selection.hit_z);
  return true;
}

// Place a block
bool player_place(Player *player, World *world) {
  if(!player || !world) return false;
  if(!player->selection.hit || !player->selection.block_hit) return false;
  if (pos_collides_specific(player->position, player->hitbox_dims, player->selection.last_x, player->selection.last_y, player->selection.last_z)) return false;

  world_set_block(world, BlockStone, player->selection.last_x, player->selection.last_y, player->selection.last_z);
  return true;
}

