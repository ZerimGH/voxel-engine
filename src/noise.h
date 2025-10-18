#ifndef NOISE_H
#define NOISE_H

// Includes
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

// Structs

// Function prototypes
// Generate a single 2D noise value at a resolution
float noise_2d(float x, float z, int res, uint32_t seed);
// Layer 2D noise with varying amplitudes and frequencies
float octave_noise_2d(float x, float z, int octaves, float persistence,
                      float lacunarity, int base_res, uint32_t seed);
// Generate a single 3D noise value at a resolution
float noise_3d(float x, float y, float z, int res, uint32_t seed);
// Layer 3D noise with varying amplitudes and frequencies
float octave_noise_3d(float x, float y, float z, int octaves, float persistence,
                      float lacunarity, int base_res, uint32_t seed);

#endif // noise.h
