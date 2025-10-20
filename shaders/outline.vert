#version 330 core

layout(location = 0) in vec3 aPos;

uniform vec3 uPos;
uniform bool uActive;
uniform mat4 uMVP;

void main() {
  if(uActive) {
    gl_Position = uMVP * vec4(aPos + uPos, 1);
  } else {
    gl_Position = vec4(0, 0, 0, 0);
  }
}
