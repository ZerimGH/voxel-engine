#ifndef CROSSHAIR_H

#define CROSSHAIR_H

#include "nuGL.h"
#include "ui_renderer.h"

typedef struct {
  nu_Texture *texture;
} Crosshair;

Crosshair *create_crosshair(void);
void destroy_crosshair(Crosshair **crosshair);
void render_crosshair(Crosshair *crosshair, UiRenderer *ui, float w, float h);

#endif
