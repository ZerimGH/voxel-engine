#ifndef PLAYER_H

#define PLAYER_H

#define PLAYER_MAX_HEALTH 20

#include "camera.h"
#include "cglm/cglm.h"
#include "world.h"
#include "block.h"

typedef struct {
  // Player's camera 
  Camera *camera;
  // Size of the players hitbox, in blocks
  vec3 hitbox_dims;
  // Player's position, in blocks
  vec3 position;
  // Player's velocity, in blocks per second
  vec3 velocity;
  // Accumulates movement directions so they can be normalised
  vec3 movement;

  // Track if the player is on the ground
  bool grounded;
  bool last_grounded;

  // Movement flags
  bool is_crouching;
  bool is_sprinting;
  bool is_jumping;
  bool keep_sprint;
  float jump_time;

  // Interpolation for animations
  float sprint_interp;
  float crouch_interp;

  float dt; // Deltatime, should be set each frame
} Player;

// Create a player
Player *create_player(void);
// Destroy a player
void destroy_player(Player **player);
// Rotate a player based on mouse movement
void player_rotate(Player *player, double dx, double dy);
// WASD
void player_forwards(Player *player);
void player_backwards(Player *player);
void player_right(Player *player);
void player_left(Player *player);
// Other movement 
void player_set_jumping(Player *player, bool state);
void player_set_crouching(Player *player, bool state);
void player_set_sprinting(Player *player, bool state);
// Break / place blocks
bool player_break(Player *player, World *world);
bool player_place(Player *player, World *world);
// Update a player each frame
void player_update(Player *player, World *world);

#endif // player.h
