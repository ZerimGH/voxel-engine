#include "clouds.h"

#define CLOUDS_SCALE 8192.f
#define CLOUDS_HEIGHT 200.f

typedef struct {
  GLfloat pos[3];
  GLfloat tex[2];
} CloudVertex;

size_t cloud_num_comp = 2;
size_t cloud_sizes[] = {sizeof(GLfloat), sizeof(GLfloat)};
size_t cloud_counts[] = {3, 2};
GLenum cloud_types[] = {GL_FLOAT, GL_FLOAT};

Clouds *create_clouds(void) {
  // Create program
  nu_Program *program = nu_create_program(2, "shaders/clouds.vert", "shaders/clouds.frag");
  if(!program) {
    fprintf(stderr, "(create_clouds): Error creating clouds, nu_create_program() returned NULL.\n");
    return NULL;
  }
  nu_register_uniform(program, "uTexture", GL_INT);
  nu_register_uniform(program, "uScale", GL_FLOAT);
  nu_register_uniform(program, "uTime", GL_FLOAT);
  nu_register_uniform(program, "uOff", GL_FLOAT_VEC3);
  nu_register_uniform(program, "uMVP", GL_FLOAT_MAT4);
  int slot = 0;
  nu_set_uniform(program, "uTexture", &slot);
  float scale = CLOUDS_SCALE;
  nu_set_uniform(program, "uScale", &scale);

  // Load texture
  nu_Texture *texture = nu_load_texture("textures/clouds.png");
  if(!texture) {
    fprintf(stderr, "(create_clouds): Error creating clouds, nu_load_texture() returned NULL.\n");
    nu_destroy_program(&program);
    return NULL;
  }

  // Create mesh
  nu_Mesh *mesh = nu_create_mesh(cloud_num_comp, cloud_sizes, cloud_counts, cloud_types);
  if(!mesh) {
    fprintf(stderr, "(create_clouds): Error creating clouds, nu_create_mesh() returned NULL.\n");
    nu_destroy_program(&program);
    nu_destroy_texture(&texture);
  }
  CloudVertex quad_mesh[] = {
    {{-1, 0, -1}, {0, 0}}, 
    {{1, 0, -1}, {1, 0}}, 
    {{-1, 0, 1}, {0, 1}}, 
    {{1, 0, 1}, {1, 1}}, 
    {{1, 0, -1}, {1, 0}}, 
    {{-1, 0, 1}, {0, 1}} 
  };
  nu_mesh_add_bytes(mesh, sizeof(quad_mesh), quad_mesh);
  nu_send_mesh(mesh);
  nu_free_mesh(mesh);

  // Create clouds
  Clouds *clouds = calloc(1, sizeof(Clouds));
  if(!clouds) {
    fprintf(stderr, "(create_clouds): Error creating clouds, calloc failed.\n");
    nu_destroy_program(&program);
    nu_destroy_texture(&texture);
    nu_destroy_mesh(&mesh);
    return NULL;
  }

  // Set members
  clouds->program = program;
  clouds->texture = texture;
  clouds->mesh = mesh;

  return clouds;
}

void destroy_clouds(Clouds **clouds) {
  if(!clouds || !(*clouds)) return;
  nu_destroy_program(&(*clouds)->program);
  nu_destroy_texture(&(*clouds)->texture);
  nu_destroy_mesh(&(*clouds)->mesh);
  free(*clouds);
  *clouds = NULL;
}
void render_clouds(Clouds *clouds, Player *player, float time, float aspect) {
  if(!clouds || !player) return;
  // Set uniforms
  float off[3] = {player->position[0], CLOUDS_HEIGHT, player->position[2]};
  nu_set_uniform(clouds->program, "uOff", off);
  nu_set_uniform(clouds->program, "uTime", &time);
  float old_near = player->camera->near;
  float old_far = player->camera->far;
  // player->camera->near = 10.f;
  player->camera->far = ((float)CLOUDS_SCALE) * 1.415f;
  mat4 vp;
  camera_calculate_vp_matrix(player->camera, vp, aspect);
  player->camera->near = old_near;
  player->camera->far = old_far;
  nu_set_uniform(clouds->program, "uMVP", &vp[0][0]);
  // Render
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  nu_bind_texture(clouds->texture, 0);
  nu_use_program(clouds->program);
  nu_render_mesh(clouds->mesh);
  // TODO
} 
