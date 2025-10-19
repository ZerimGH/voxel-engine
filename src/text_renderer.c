#include "text_renderer.h"

#define NUM_CHARACTERS 11

#define CHARACTER_TEXTURES                                                     \
  "textures/unknown.png", "textures/0.png", "textures/1.png",                  \
      "textures/2.png", "textures/3.png", "textures/4.png", "textures/5.png",  \
      "textures/6.png", "textures/7.png", "textures/8.png", "textures/9.png"

TextRenderer *create_text_renderer(void) {
  nu_Texture *texture =
      nu_load_texture_array(NUM_CHARACTERS, CHARACTER_TEXTURES);
  if (!texture) {
    fprintf(stderr, "(create_text_renderer): Error creating text renderer, "
                    "nu_load_texture_array() returned NULL.\n");
    return NULL;
  }

  TextRenderer *text = calloc(1, sizeof(TextRenderer));
  if (!text) {
    fprintf(stderr, "(create_text_renderer): Error creating text renderer, "
                    "calloc failed.\n");
    nu_destroy_texture(&texture);
    return NULL;
  }

  text->texture = texture;
  return text;
}

void destroy_text_renderer(TextRenderer **text) {
  if (!text || !(*text))
    return;
  nu_destroy_texture(&(*text)->texture);
  free(*text);
  *text = NULL;
}

static int char_to_index(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0' + 1;
  } else
    return 0;
}

static void text_render_indices(TextRenderer *text, UiRenderer *ui, float x,
                                float y, float size, float padding,
                                float screen_width, float screen_height,
                                size_t num_indices, int *indices) {
  if (!text || !ui || !indices || num_indices == 0)
    return;
  nu_bind_texture(text->texture, 1);
  ui_use_array(ui, true);
  for (size_t i = 0; i < num_indices; i++) {
    float pos_x = x + (size + padding) * i;
    float pos_y = y;
    ui_set_index(ui, indices[i]);
    ui_render_quad(ui, pos_x, pos_y, size, size, screen_width, screen_height);
  }
}

void text_render_string(TextRenderer *text, UiRenderer *ui, float x, float y,
                        float size, float padding, float screen_width,
                        float screen_height, char *str) {
  if (!text || !ui)
    return;
  if (!str)
    str = "null";

  size_t len = strlen(str);
  int *indices = calloc(len, sizeof(int));
  if (!indices) {
    fprintf(
        stderr,
        "(text_render_string): Couldn't render string \"%s\", calloc failed.\n",
        str);
    return;
  }

  for (size_t i = 0; i < len; i++) {
    indices[i] = char_to_index(str[i]);
  }

  text_render_indices(text, ui, x, y, size, padding, screen_width,
                      screen_height, len, indices);

  free(indices);
}

void text_render_number(TextRenderer *text, UiRenderer *ui, float x, float y,
                        float size, float padding, float screen_width,
                        float screen_height, int n) {
  char tmp[1024];
  sprintf(tmp, "%d", n);
  text_render_string(text, ui, x, y, size, padding, screen_width, screen_height,
                     tmp);
}
