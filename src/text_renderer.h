#ifndef TEXT_RENDERER_H

#define TEXT_RENDERER_H

#include "nuGL.h"
#include "ui_renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  nu_Texture *texture;
} TextRenderer;

TextRenderer *create_text_renderer(void);
void destroy_text_renderer(TextRenderer **text);
void text_render_number(TextRenderer *text, UiRenderer *ui, float x, float y,
                        float size, float padding, float screen_width,
                        float screen_height, int n);

#endif
