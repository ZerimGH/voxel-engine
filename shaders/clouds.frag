#version 330 core

in vec2 fTex;
out vec4 fragColor;

uniform sampler2D uTexture;

void main() {
  vec4 col = texture(uTexture, fTex);
  if(col.a < 0.005f) discard;
  fragColor = col;
}
