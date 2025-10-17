#include "ui_renderer.h"

typedef struct {
  GLfloat pos[2];
  GLfloat tex[2];
} QuadVertex;

size_t quad_num_components = 2;
size_t quad_sizes[] = {sizeof(GLfloat), sizeof(GLfloat)};
size_t quad_counts[] = {2, 2};
GLenum quad_types[] = {GL_FLOAT, GL_FLOAT};

UiRenderer *create_ui_renderer(void) {
  nu_Mesh *mesh = nu_create_mesh(quad_num_components, quad_sizes, quad_counts, quad_types); 
  if(!mesh) {
    fprintf(stderr, "(create_ui_renderer): Couldn't create ui renderer, nu_create_mesh() returned NULL.\n");
    return NULL;
  }

  QuadVertex quad[6] = {
    {{-1, -1}, {0, 0}},
    {{1, -1}, {1, 0}},
    {{-1, 1}, {0, 1}},
    {{1, 1}, {1, 1}},
    {{1, -1}, {1, 0}},
    {{-1, 1}, {0, 1}}
  };

  nu_mesh_add_bytes(mesh, sizeof(quad), quad);
  nu_send_mesh(mesh);
  nu_free_mesh(mesh);

  nu_Program *program = nu_create_program(2, "shaders/ui.vert", "shaders/ui.frag");
  if(!program) {
    fprintf(stderr, "(create_ui_renderer): Couldn't create ui renderer, nu_create_program() returned NULL.\n");
    nu_destroy_mesh(&mesh);
    return NULL;
  }
  nu_register_uniform(program, "uTexture", GL_INT);
  nu_register_uniform(program, "uPos", GL_FLOAT_VEC2);
  nu_register_uniform(program, "uScale", GL_FLOAT_VEC2);
  int unit = 0;
  nu_set_uniform(program, "uTexture", &unit);

  UiRenderer *ui = calloc(1, sizeof(UiRenderer));
  if(!ui) {
    fprintf(stderr, "(create_ui_renderer): Couldn't create ui renderer, calloc failed.\n");
    nu_destroy_mesh(&mesh);
    nu_destroy_program(&program);
    return NULL;
  }
  ui->quad_mesh = mesh;
  ui->program = program;
  return ui;
}
void destroy_ui_renderer(UiRenderer **ui) {
  if(!ui || !(*ui)) return; 
  nu_destroy_mesh(&(*ui)->quad_mesh);
  nu_destroy_program(&(*ui)->program);
  free(*ui);
  *ui = NULL;
}

static inline float map(float n, float n_min, float n_max, float new_min, float new_max) {
  if(n_min == n_max) return new_min; // Avoid div by 0
  n -= n_min;
  n /= (n_max - n_min);
  n *= (new_max - new_min);
  n += new_min;
  return n;
}

void ui_render_quad(UiRenderer *ui, float x, float y, float w, float h, float screen_width, float screen_height) {
  if(!ui) return;
  // Convert coordinates to -1 -> 1
  float x_frac = x / screen_width;
  float y_frac = y / screen_height;

  float w_frac = w / screen_width;
  float h_frac = h / screen_height;

  float screen_x = map(x_frac, 0, 1, -1, 1);
  float screen_y = map(y_frac, 0, 1, -1, 1);

  float screen_w = w_frac;
  float screen_h = h_frac;

  float uPos[2] = {screen_x, screen_y};
  float uScale[2] = {screen_w, screen_h};
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  nu_set_uniform(ui->program, "uPos", uPos);
  nu_set_uniform(ui->program, "uScale", uScale);
  nu_use_program(ui->program);
  nu_render_mesh(ui->quad_mesh);
  glDepthMask(GL_TRUE);
}

