#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTex;

uniform mat4 uMVP; 

out vec2 fTex;

void main() {
  gl_Position = uMVP * vec4(aPosition, 1.0);
  fTex = aTex;
}
