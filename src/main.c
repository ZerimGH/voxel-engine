#include <stdio.h>
#define NUGL_DEBUG
#include "camera.h"
#include "chunk.h"
#include "nuGL.h"
#include "profiler.h"

#define SENS 0.0015f
#define MOVE_SPEED 0.3f

int main(void) {
  nu_Window *window = nu_create_window(600, 600, NULL, NULL);
  glfwSetInputMode(window->glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  nu_Program *program = nu_create_program(2, "shaders/block.vert", "shaders/block.frag");
  nu_register_uniform(program, "uMVP", GL_FLOAT_MAT4);
  Chunk *chunk = create_chunk(0, 0, 0);
  generate_chunk(chunk);
  START_TIMER(mesh_chunk);
  mesh_chunk(chunk);
  END_TIMER(mesh_chunk);
  nu_print_mesh(chunk->mesh);

  // Create a camera
  Camera *cam = create_camera(0.1, 200, 90.f);

  while (!glfwWindowShouldClose(window->glfw_window)) {
    // Update input
    if (nu_get_key_state(window, GLFW_KEY_S)) camera_move(cam, (vec3){0, 0, -MOVE_SPEED});
    if (nu_get_key_state(window, GLFW_KEY_W)) camera_move(cam, (vec3){0, 0, MOVE_SPEED});
    if (nu_get_key_state(window, GLFW_KEY_A)) camera_move(cam, (vec3){-MOVE_SPEED, 0, 0});
    if (nu_get_key_state(window, GLFW_KEY_D)) camera_move(cam, (vec3){MOVE_SPEED, 0, 0});
    if (nu_get_key_state(window, GLFW_KEY_SPACE)) camera_move(cam, (vec3){0, MOVE_SPEED, 0});
    if (nu_get_key_state(window, GLFW_KEY_LEFT_SHIFT)) camera_move(cam, (vec3){0, -MOVE_SPEED, 0});
    camera_rotate(cam, nu_get_delta_mouse_x(window) * SENS, -nu_get_delta_mouse_y(window) * SENS);
    nu_update_input(window);

    // Send uniforms
    mat4 vp;
    camera_calculate_vp_matrix(cam, vp, (float)window->width / (float)window->height);
    nu_set_uniform(program, "uMVP", &vp[0][0]);

    // Rendering
    nu_start_frame(window);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nu_use_program(program);
    nu_render_mesh(chunk->mesh);
    nu_end_frame(window);
  }
  nu_destroy_window(&window);
  nu_destroy_program(&program);
  destroy_camera(&cam);
  destroy_chunk(&chunk);
  return 0;
}
