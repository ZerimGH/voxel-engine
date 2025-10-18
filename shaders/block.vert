#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTex;
layout(location = 2) in int aSideIndex;
layout(location = 3) in int aBlockType;

uniform mat4 uMVP; 
uniform vec3 uPlayerPos;
uniform float uRenderDistance;

out vec2 fTex;
out float brightness;
flat out int texture_index;
out float fFogFactor;

void main() {
  gl_Position = uMVP * vec4(aPosition, 1.0);
  fTex = aTex;
  float direction_light = 1.f;
  float base_brightness = 1.f;
  if(aSideIndex == 0) direction_light = 0.8f;
  else if(aSideIndex == 1) direction_light = 0.6f;
  else if(aSideIndex == 2) direction_light = 0.6f;
  else if(aSideIndex == 3) direction_light = 0.8;
  else if(aSideIndex == 4) direction_light = 1.f;
  else if(aSideIndex == 5) direction_light = 0.5f;
  else direction_light = 0.2f;
  int texture_offset = 0;
  if(aSideIndex < 4) texture_offset = 1;
  else if(aSideIndex == 4) texture_offset = 0;
  else texture_offset = 2;
  brightness = direction_light * base_brightness;
  texture_index = (aBlockType - 1) * 3 + texture_offset; // -1 for air
  float fog_near = uRenderDistance / 2.f;
  float fog_far = uRenderDistance;
  float dist = distance(aPosition, uPlayerPos);
  dist = clamp(dist, fog_near, fog_far);
  fFogFactor = (dist - fog_near) / (fog_far - fog_near); 
}
