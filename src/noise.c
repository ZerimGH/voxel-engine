#include "noise.h"

// https://gist.github.com/badboy/6267743
static inline uint32_t hash6432shift(uint64_t key, uint32_t seed) {
  key += (uint64_t)seed * 0x9e3779b97f4a7c15ULL;
  key = (~key) + (key << 18);
  key ^= (key >> 31);
  key *= 21;
  key ^= (key >> 11);
  key += (key << 6);
  key ^= (key >> 22);
  key ^= (uint64_t)seed * 0x27d4eb2dULL;
  return (uint32_t)key;
}

// Hash a pair of coordinates
static inline int hash_coords_2d(int x, int y, uint32_t seed) {
  uint64_t lx = (uint64_t)(uint32_t)x;
  uint64_t ly = (uint64_t)(uint32_t)y;
  uint64_t n = (lx << 32) | ly;
  return (int)hash6432shift(n, seed);
}

// Hash 3 coordinates
static inline int hash_coords_3d(int x, int y, int z, uint32_t seed) {
  uint64_t n = (uint64_t)x * 73856093ULL ^ (uint64_t)y * 19349663ULL ^ (uint64_t)z * 83492791ULL;
  return (int)hash6432shift(n, seed);
}

static inline float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

// Generate a single 2D noise value at a resolution
float noise_2d(float x, float z, int res, uint32_t seed) {
  int xa = (int)floorf(x / res);
  int za = (int)floorf(z / res);
  int xb = xa + 1;
  int zb = za + 1;

  float tx = (x - xa * res) / res;
  float tz = (z - za * res) / res;

  const float inv_int_max = 1.0f / (float)INT32_MAX;
  float zah = (hash_coords_2d(xa, za, seed) & 0x7fffffff) * inv_int_max;
  float zbh = (hash_coords_2d(xb, za, seed) & 0x7fffffff) * inv_int_max;
  float xah = (hash_coords_2d(xa, zb, seed) & 0x7fffffff) * inv_int_max;
  float xbh = (hash_coords_2d(xb, zb, seed) & 0x7fffffff) * inv_int_max;

  float top = lerp(zah, zbh, tx);
  float bottom = lerp(xah, xbh, tx);
  return lerp(top, bottom, tz) * 2 - 1;
}

// Layer 2D noise with varying amplitudes and frequencies
float octave_noise_2d(float x, float z, int octaves, float persistence, float lacunarity, int base_res, uint32_t seed) {
  float total = 0.0f, frequency = 1.0f, amplitude = 1.0f, maxValue = 0.0f;

  for (int i = 0; i < octaves; i++) {
    int res = (int)fmaxf(1.0f, base_res / frequency);
    total += noise_2d(x * frequency + i * 54209, z * frequency + i * 82731, res, seed) * amplitude;
    maxValue += amplitude;
    amplitude *= persistence;
    frequency *= lacunarity;
  }

  return total / maxValue;
}

// Generate a single 3D noise value at a resolution
float noise_3d(float x, float y, float z, int res, uint32_t seed) {
  int xa = (int)floorf(x / res);
  int ya = (int)floorf(y / res);
  int za = (int)floorf(z / res);
  int xb = xa + 1;
  int yb = ya + 1;
  int zb = za + 1;

  float tx = (x - xa * res) / res;
  float ty = (y - ya * res) / res;
  float tz = (z - za * res) / res;

  const float inv_int_max = 1.0f / (float)INT32_MAX;

  float v000 = (hash_coords_3d(xa, ya, za, seed) & 0x7fffffff) * inv_int_max;
  float v100 = (hash_coords_3d(xb, ya, za, seed) & 0x7fffffff) * inv_int_max;
  float v010 = (hash_coords_3d(xa, yb, za, seed) & 0x7fffffff) * inv_int_max;
  float v110 = (hash_coords_3d(xb, yb, za, seed) & 0x7fffffff) * inv_int_max;
  float v001 = (hash_coords_3d(xa, ya, zb, seed) & 0x7fffffff) * inv_int_max;
  float v101 = (hash_coords_3d(xb, ya, zb, seed) & 0x7fffffff) * inv_int_max;
  float v011 = (hash_coords_3d(xa, yb, zb, seed) & 0x7fffffff) * inv_int_max;
  float v111 = (hash_coords_3d(xb, yb, zb, seed) & 0x7fffffff) * inv_int_max;

  float x00 = lerp(v000, v100, tx);
  float x10 = lerp(v010, v110, tx);
  float x01 = lerp(v001, v101, tx);
  float x11 = lerp(v011, v111, tx);

  float y0 = lerp(x00, x10, ty);
  float y1 = lerp(x01, x11, ty);

  return lerp(y0, y1, tz) * 2.0f - 1.0f;
}

// Layer 3D noise with varying amplitudes and frequencies
float octave_noise_3d(float x, float y, float z, int octaves, float persistence, float lacunarity, int base_res, uint32_t seed) {
  float total = 0.0f, frequency = 1.0f, amplitude = 1.0f, maxValue = 0.0f;

  for (int i = 0; i < octaves; i++) {
    int res = (int)fmaxf(1.0f, base_res / frequency);
    total += noise_3d(x * frequency + i * 54209, y * frequency + i * 129871, z * frequency + i * 82731, res, seed) * amplitude;
    maxValue += amplitude;
    amplitude *= persistence;
    frequency *= lacunarity;
  }

  return total / maxValue;
}
