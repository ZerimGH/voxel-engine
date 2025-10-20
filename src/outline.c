#include "outline.h"

typedef struct {
  GLfloat pos[3];
} OutlineVertex;

size_t outline_num = 1;
size_t outline_sizes[] = {sizeof(GLfloat)};
size_t outline_counts[] = {3};
GLenum outline_types[] = {GL_FLOAT};

OutlineRenderer *create_outline_renderer(void) {
  // Create shader program for outline
  nu_Program *program = nu_create_program(2, "shaders/outline.vert", "shaders/outline.frag");
  if(!program) {
    fprintf(stderr, "(create_outline_renderer): Couldn't create outline renderer, nu_create_program() returned NULL.\n");
    return NULL;
  }
  nu_register_uniform(program, "uPos", GL_FLOAT_VEC3);
  nu_register_uniform(program, "uActive", GL_INT);
  nu_register_uniform(program, "uMVP", GL_FLOAT_MAT4);

  // Create mesh
  nu_Mesh *mesh = nu_create_mesh(outline_num, outline_sizes, outline_counts, outline_types);
  if(!mesh) {
    nu_destroy_program(&program);
    fprintf(stderr, "(create_outline_renderer): Couldn't create outline renderer, nu_create_mesh() returned NULL.\n");
    return NULL;
  }
  OutlineVertex vertices[] = {
      // front face (z = 0)
      {{0, 0, 0}},
      {{1, 0, 0}},
      {{1, 0, 0}},
      {{1, 1, 0}},
      {{1, 1, 0}},
      {{0, 1, 0}},
      {{0, 1, 0}},
      {{0, 0, 0}},

      // back face (z = 1)
      {{0, 0, 1}},
      {{1, 0, 1}},
      {{1, 0, 1}},
      {{1, 1, 1}},
      {{1, 1, 1}},
      {{0, 1, 1}},
      {{0, 1, 1}},
      {{0, 0, 1}},

      // left face (x = 0)
      {{0, 0, 0}},
      {{0, 0, 1}},
      {{0, 0, 1}},
      {{0, 1, 1}},
      {{0, 1, 1}},
      {{0, 1, 0}},
      {{0, 1, 0}},
      {{0, 0, 0}},

      // right face (x = 1)
      {{1, 0, 0}},
      {{1, 0, 1}},
      {{1, 0, 1}},
      {{1, 1, 1}},
      {{1, 1, 1}},
      {{1, 1, 0}},
      {{1, 1, 0}},
      {{1, 0, 0}},
  };
  nu_mesh_add_bytes(mesh, sizeof(vertices), vertices);
  nu_send_mesh(mesh);
  nu_free_mesh(mesh);
  nu_mesh_set_render_mode(mesh, GL_LINES);

  // Create outline renderer
  OutlineRenderer *outline = calloc(1, sizeof(OutlineRenderer));
  if(!outline) {
    fprintf(stderr, "(create_outline_renderer): Couldn't create outline renderer, calloc failed.\n");
    nu_destroy_program(&program);
    nu_destroy_mesh(&mesh);
    return NULL;
  }

  outline->program = program;
  outline->mesh = mesh;
  return outline;
}

void destroy_outline_renderer(OutlineRenderer **outline) {
  if(!outline || !(*outline)) return;
  nu_destroy_program(&(*outline)->program);
  nu_destroy_mesh(&(*outline)->mesh);
  free(*outline);
  *outline = NULL;
}

void render_outline(OutlineRenderer *outline, Player *player, float width, float height) {
  if(!outline || !player) return;
  // Set uniforms
  GLfloat uPos[3] = {(GLfloat) player->selection.hit_x, (GLfloat) player->selection.hit_y, (GLfloat) player->selection.hit_z}; 
  GLint uActive = player->selection.hit;
  mat4 vp;
  camera_calculate_vp_matrix(player->camera, vp, width / height);

  nu_set_uniform(outline->program, "uPos", uPos);
  nu_set_uniform(outline->program, "uActive", &uActive);
  nu_set_uniform(outline->program, "uMVP", &vp[0][0]);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glLineWidth(5.f);

  nu_use_program(outline->program);
  nu_render_mesh(outline->mesh);
}
