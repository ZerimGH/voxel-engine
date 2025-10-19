#ifndef UI_RENDERER_H
#define UI_RENDERER_H

// Includes
#include "nuGL.h"
#include <stdbool.h>
#include <stdlib.h>

// Structs
typedef struct {
  nu_Mesh *quad_mesh;
  nu_Program *program;
} UiRenderer;

// Function prototypes
UiRenderer *create_ui_renderer(void);
void destroy_ui_renderer(UiRenderer **ui);
void ui_render_quad(UiRenderer *ui, float x, float y, float w, float h,
                    float screen_width, float screen_height);
void ui_render_centred_quad(UiRenderer *ui, float x, float y, float w, float h,
                            float screen_width, float screen_height);
void ui_use_array(UiRenderer *ui, bool val);
void ui_set_index(UiRenderer *ui, int index);

#endif // ui_renderer.h
