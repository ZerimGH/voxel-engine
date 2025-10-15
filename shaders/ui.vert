#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTex;

uniform vec2 uPos;
uniform vec2 uScale;

out vec2 fTex;

void main() {
  fTex = aTex;
  gl_Position = vec4(uPos + (aPos + vec2(1, 1)) * uScale, 0.0, 1.0);
}
