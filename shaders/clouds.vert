#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTex;

uniform float uScale;
uniform vec3 uOff;

out vec2 fTex;

void main() {
  gl_Position = vec4(aPos * uScale + uOff, 1.0)
  fTex = aTex;
}
