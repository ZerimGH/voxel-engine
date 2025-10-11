#include "sky.h"

typedef struct {
  float pos[2];
} SkyVertex;

size_t sky_vertex_num = 1;
size_t sky_vertex_sizes[] = {sizeof(float)};
size_t sky_vertex_counts[] = {2};
GLenum sky_vertex_types[] = {GL_FLOAT};

SkyRenderer *create_sky_renderer(void) {
  nu_Program *program = nu_create_program(2, "shaders/sky.vert", "shaders/sky.frag");
  nu_Mesh *mesh = nu_create_mesh(sky_vertex_num, sky_vertex_sizes, sky_vertex_counts, sky_vertex_types);
  if (!program || !mesh) {
    fprintf(stderr, "(create_sky) Error creating sky.\n");
    fprintf(stderr, "program: %p, mesh: %p\n", program, mesh);
    nu_destroy_program(&program);
    nu_destroy_mesh(&mesh);
  }
  SkyVertex quad_vertices[6] = {
      {{-1, -1}}, {{1, -1}}, {{-1, 1}}, {{1, 1}}, {{1, -1}}, {{-1, 1}},
  };
  nu_mesh_add_bytes(mesh, sizeof(quad_vertices), quad_vertices);
  nu_send_mesh(mesh);
  nu_free_mesh(mesh);

  nu_register_uniform(program, "screenHeight", GL_FLOAT);
  nu_register_uniform(program, "cameraPitch", GL_FLOAT);
  nu_register_uniform(program, "cameraFOV", GL_FLOAT);

  SkyRenderer *renderer = calloc(1, sizeof(SkyRenderer));
  if (!renderer) {
    fprintf(stderr, "(create_sky): Error creating sky, calloc failed.\n");
    nu_destroy_program(&program);
    nu_destroy_mesh(&mesh);
    return NULL;
  }
  renderer->program = program;
  renderer->mesh = mesh;
  return renderer;
}

void render_sky(SkyRenderer *renderer, float screen_height, float pitch, float fov) {
  if (!renderer) return;
  // Set OpenGL stuff
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  // Send uniforms
  nu_set_uniform(renderer->program, "screenHeight", &screen_height);
  nu_set_uniform(renderer->program, "cameraPitch", &pitch);
  nu_set_uniform(renderer->program, "cameraFOV", &fov);
  nu_use_program(renderer->program);
  nu_render_mesh(renderer->mesh);
  // Clear depth buffer, the sky is a background
  glClear(GL_DEPTH_BUFFER_BIT);
}

void destroy_sky_renderer(SkyRenderer **renderer) {
  if (!renderer || !(*renderer)) {
    return;
  }
  nu_destroy_program(&(*renderer)->program);
  nu_destroy_mesh(&(*renderer)->mesh);
  free(*renderer);
  *renderer = NULL;
}
