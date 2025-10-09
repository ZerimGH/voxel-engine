#ifndef CAMERA_H

#define CAMERA_H

#include "cglm/cglm.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
  vec3 position;
  float yaw, pitch;
  float fov;
  float near, far;
} Camera;

// function prototypes
// Allocate and return a pointer to a camera
Camera *create_camera(float near, float far, float fov);
// Destroy a camera
void destroy_camera(Camera **camera);
// calculate the vp matrix of the camera, and store in res
// aspect should be the screen width / the screen height
void camera_calculate_vp_matrix(Camera *cam, mat4 res, float aspect);
// rotate the camera by a given delta_yaw and delta_pitch
void camera_rotate(Camera *cam, float dy, float dp);
// translate the camera by global dx, dy, dx
void camera_translate(Camera *cam, vec3 d_pos);
// move the camera by relative (right, up, forward) vec
void camera_move(Camera *cam, vec3 dir);
// get the forward direction of the camera as a vec3
void camera_calculate_forward(Camera *camera, vec3 res);
// get the right direction of the camera as a vec3
void camera_calculate_right(Camera *camera, vec3 res);
#endif // camera.h
