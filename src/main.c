#include <assert.h>
#include <limits.h>
#include <stdio.h>
#define NUGL_DEBUG
#include "block.h"
#include "camera.h"
#include "chunk.h"
#include "nuGL.h"
#include "profiler.h"

#define SENS 0.0015f
#define MOVE_SPEED 0.3f

int main(void) {
  assert((long)CHUNK_WIDTH * (long)CHUNK_HEIGHT * (long)CHUNK_LENGTH < INT_MAX / 100); // Make sure chunk size doesnt overflow anywhere

  // Create the window
  nu_Window *window = nu_create_window(600, 600, NULL, NULL);
  if (!window) return 1; // Return on failure to create window
  glfwSetInputMode(window->glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  // Create shader program
  nu_Program *program = nu_create_program(2, "shaders/block.vert", "shaders/block.frag");
  nu_register_uniform(program, "uMVP", GL_FLOAT_MAT4);
  nu_register_uniform(program, "uTexture", GL_INT);
  // Load tetxure images
  nu_Texture *textures = nu_load_texture_array(NUM_BLOCK_TEXTURES, BLOCK_TEXTURES);
  int texture_unit = 0;
  nu_set_uniform(program, "uTexture", &texture_unit);
  // Create, generate and mesh a single chunk
  Chunk *chunk = create_chunk(0, 0, 0);
  print_chunk(chunk);
  START_TIMER(generate_chunk);
  generate_chunk(chunk);
  END_TIMER(generate_chunk);
  START_TIMER(mesh_chunk);
  mesh_chunk(chunk);
  END_TIMER(mesh_chunk);

  // Debug prints
  nu_print_window(window, 0);
  nu_print_program(program, 0);
  nu_print_texture(textures, 0);
  print_chunk(chunk);

  // Create a camera
  Camera *cam = create_camera(0.1, 512, 90.f);

  while (!glfwWindowShouldClose(window->glfw_window)) {
    // Update input
    if (nu_get_key_state(window, GLFW_KEY_S)) camera_move(cam, (vec3){0, 0, -MOVE_SPEED});
    if (nu_get_key_state(window, GLFW_KEY_W)) camera_move(cam, (vec3){0, 0, MOVE_SPEED});
    if (nu_get_key_state(window, GLFW_KEY_A)) camera_move(cam, (vec3){-MOVE_SPEED, 0, 0});
    if (nu_get_key_state(window, GLFW_KEY_D)) camera_move(cam, (vec3){MOVE_SPEED, 0, 0});
    if (nu_get_key_state(window, GLFW_KEY_SPACE)) camera_move(cam, (vec3){0, MOVE_SPEED, 0});
    if (nu_get_key_state(window, GLFW_KEY_LEFT_SHIFT)) camera_move(cam, (vec3){0, -MOVE_SPEED, 0});
    camera_rotate(cam, nu_get_delta_mouse_x(window) * SENS, -nu_get_delta_mouse_y(window) * SENS);
    nu_update_input(window); // Idk why i have to do this after using input?

    // Send uniforms
    mat4 vp;
    camera_calculate_vp_matrix(cam, vp, (float)window->width / (float)window->height);
    nu_set_uniform(program, "uMVP", &vp[0][0]);

    // Rendering
    nu_start_frame(window);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.1f, 0.85f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nu_use_program(program);
    nu_bind_texture(textures, 0);
    nu_render_mesh(chunk->mesh);
    nu_end_frame(window);
  }
  printf("Exiting...\n");

  // Cleanup
  nu_destroy_window(&window);
  nu_destroy_program(&program);
  nu_destroy_texture(&textures);
  destroy_camera(&cam);
  destroy_chunk(&chunk);

  nu_print_window(window, 0);
  nu_print_program(program, 0);
  print_chunk(chunk);
  return 0;
}
