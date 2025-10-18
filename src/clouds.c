#include "clouds.h"

#define CLOUDS_SCALE 512.f
#define CLOUDS_HEIGHT 100.f

typedef struct {
  GLfloat pos[3];
  GLfloat tex[2];
} CloudVertex;

size_t cloud_num_comp = 2;
size_t cloud_sizes[] = {sizeof(GLfloat), sizeof(GLfloat)};
size_t cloud_counts[] = {3, 2};
GLenum cloud_types[] = {GL_FLOAT, GL_FLOAT};

Clouds *create_clouds(void) {
  nu_Program *program = nu_create_program(2, "shaders/clouds.verts", "shaders/clouds.frag");
  if(!program) {
    fprintf(stderr, "(create_clouds): Error creating clouds, nu_create_program() returned NULL.\n");
    return NULL;
  }

  nu_register_uniform(program, "uTexture", GL_INT);
  nu_register_uniform(program, "uScale", GL_FLOAT);
  nu_register_uniform(program, "uOff", GL_FLOAT_VEC3);

  int slot = 0;
  nu_set_uniform(program, "uTexture", &slot);

  float scale = CLOUDS_SCALE;
  nu_set_uniform(program, "uScale", &scale);

  nu_Texture *texture = nu_load_texture("textures/hyrax.jpg");
  if(!texture) {
    fprintf(stderr, "(create_clouds): Error creating clouds, nu_load_texture() returned NULL.\n");
    nu_destroy_program(&program);
    return NULL;
  }

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

  Clouds *clouds = calloc(1, sizeof(Clouds));
  if(!clouds) {
    fprintf(stderr, "(create_clouds): Error creating clouds, calloc failed.\n");
    nu_destroy_program(&program);
    nu_destroy_texture(&texture);
    nu_destroy_mesh(&mesh);
    return NULL;
  }

  clouds->program = program;
  clouds->texture = texture;

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
void render_clouds(Clouds *clouds, Player *player, float time) {
  if(!clouds || !player) return;
  /*
  float off[3] = {player->pos[0], CLOUDS_HEIGHT, player->pos[2]};
  nu_set_uniform(clouds->program, "uOff", off);
  nu_bind_texture(clouds->texture, 0);
  nu_
  */
  // TODO
} 
