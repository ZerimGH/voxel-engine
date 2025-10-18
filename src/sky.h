#ifndef SKY_H

#define SKY_H

// Includes
#include "nuGL.h"
#include <stdio.h>
#include <stdlib.h>

// Structs
typedef struct {
  nu_Program *program;
  nu_Mesh *mesh;
} SkyRenderer;

// Function prototypes
SkyRenderer *create_sky_renderer(void);
void render_sky(SkyRenderer *renderer, float screen_height, float pitch,
                float fov);
void destroy_sky_renderer(SkyRenderer **renderer);

#endif
