#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTex;

uniform float uScale;
uniform float uTime;
uniform vec3 uOff;
uniform mat4 uMVP;

out vec2 fTex;
out vec2 fWorldPos;

void main() {
  vec3 worldPos = aPos * uScale + uOff;
  gl_Position = uMVP * vec4(worldPos, 1.0);
  fTex = vec2(uOff.x - uTime, uOff.z) / (2 * uScale) + aTex;

  fWorldPos = worldPos.xz; 
}
