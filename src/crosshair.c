#include "crosshair.h"

#define CROSSHAIR_SIZE 32 // Size of the crosshair at 1920x1080

Crosshair *create_crosshair(void) {
  nu_Texture *texture = nu_load_texture("textures/crosshair.png");
  if (!texture) {
    fprintf(stderr, "(create_crosshair): Couldn't create crosshair, "
                    "nu_load_texture() returned NULL.\n");
    return NULL;
  }
  Crosshair *c = calloc(1, sizeof(Crosshair));
  if (!c) {
    nu_destroy_texture(&texture);
    fprintf(stderr,
            "(create_crosshair): Couldn't create crosshair, calloc failed.\n");
    return NULL;
  }
  c->texture = texture;
  return c;
}

void destroy_crosshair(Crosshair **crosshair) {
  if (!crosshair || !*crosshair)
    return;
  nu_destroy_texture(&(*crosshair)->texture);
  free(*crosshair);
  *crosshair = NULL;
}

void render_crosshair(Crosshair *crosshair, UiRenderer *ui, float w, float h) {
  if (!crosshair || !ui)
    return;
  // calculate size of crosshair
  float screen_size = w / 1920.f * CROSSHAIR_SIZE;
  nu_bind_texture(crosshair->texture, 0);
  ui_render_centred_quad(ui, w / 2.f, h / 2.f, screen_size, screen_size, w, h);
}
