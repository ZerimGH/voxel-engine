#include "camera.h"
#include "cglm/cglm.h"

#define PI 3.14159265f

Camera *create_camera(float near, float far, float fov) {
  Camera *camera = calloc(1, sizeof(Camera));
  if (!camera) {
    fprintf(stderr, "(create_camera): Error creating camera, calloc failed.\n");
    return NULL;
  }
  camera->near = near;
  camera->far = far;
  camera->yaw = -PI / 2;
  camera->pitch = 0.0f;
  camera->fov = fov;
  return camera;
}

void destroy_camera(Camera **camera) {
  if (!camera || !(*camera)) return;
  free(*camera);
  *camera = NULL;
}

void camera_calculate_forward(Camera *camera, vec3 res) {
  if (!camera || !res) return;
  float xz_len = cosf(camera->pitch);
  res[0] = xz_len * cosf(camera->yaw);
  res[1] = sinf(camera->pitch);
  res[2] = xz_len * sinf(camera->yaw);
}

void camera_calculate_right(Camera *camera, vec3 res) {
  if (!camera || !res) return;
  // first, calculate forward
  camera_calculate_forward(camera, res);
  // then, cross with up
  glm_vec3_cross(res, (vec3){0, 1, 0}, res);
}

void camera_calculate_vp_matrix(Camera *camera, mat4 res, float aspect) {
  if (!camera || !res) return;
  vec3 dir_vec;
  camera_calculate_forward(camera, dir_vec);
  mat4 view;
  mat4 projection;

  vec3 lookat_target;
  glm_vec3_add(camera->position, dir_vec, lookat_target);

  glm_lookat(camera->position, lookat_target, (vec3){0, 1, 0}, view);
  glm_perspective(glm_rad(camera->fov), aspect, camera->near, camera->far, projection);

  glm_mat4_mul(projection, view, res);
}

void camera_translate(Camera *camera, vec3 delta_pos) {
  if (!camera || !delta_pos) return;
  glm_vec3_add(camera->position, delta_pos, camera->position);
}

void camera_rotate(Camera *camera, float delta_yaw, float delta_pitch) {
  if (!camera) return;
  camera->yaw += delta_yaw;
  camera->pitch += delta_pitch;
  // clamp camera pitch value
  if (camera->pitch > 0.99f * PI / 2.f) camera->pitch = 0.99f * PI / 2.f;
  if (camera->pitch < 0.99f * -PI / 2.f) camera->pitch = 0.99f * -PI / 2.f;
  // make camera yaw value wrap around
  while (camera->yaw < 0) camera->yaw += PI * 2;
  while (camera->yaw > PI * 2) camera->yaw -= PI * 2;
}

static void camera_move_forward(Camera *camera, float step_size) {
  if (!camera) return;
  // calculate forward vector
  vec3 forward_vec;
  camera_calculate_forward(camera, forward_vec);
  // ignore y value
  forward_vec[1] = 0;
  // normalize
  glm_vec3_normalize(forward_vec);
  // mult by step_size
  glm_vec3_scale(forward_vec, step_size, forward_vec);
  // add to camera's current pos
  camera_translate(camera, forward_vec);
}

static void camera_move_right(Camera *camera, float step_size) {
  if (!camera) return;
  // calculate right vector
  vec3 right_vec;
  camera_calculate_right(camera, right_vec);
  // ignore y value
  right_vec[1] = 0;
  // normalize
  glm_vec3_normalize(right_vec);
  // mult by step_size
  glm_vec3_scale(right_vec, step_size, right_vec);
  // add to camera's current pos
  camera_translate(camera, right_vec);
}

static void camera_move_up(Camera *camera, float step_size) {
  if (!camera) return;
  // up vector is constant
  vec3 up_vec = {0, 1, 0};
  // mult by step_size
  glm_vec3_scale(up_vec, step_size, up_vec);
  // add to camera's current pos
  camera_translate(camera, up_vec);
}

// move the camera with a (right, up, forward) vector, as opposed to global
// (dx, dy, dz) vector
void camera_move(Camera *camera, vec3 dir) {
  if (!camera || !dir) return;
  camera_move_right(camera, dir[0]);
  camera_move_up(camera, dir[1]);
  camera_move_forward(camera, dir[2]);
}
