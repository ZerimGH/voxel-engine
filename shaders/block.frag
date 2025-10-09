#version 330 core

out vec4 fragColor;

in vec2 fTex;

void main() {
  fragColor = vec4(fTex.x, 0, fTex.y, 1);
}
