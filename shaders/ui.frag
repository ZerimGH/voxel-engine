#version 330 core

in vec2 fTex;

uniform sampler2D uTexture;

out vec4 fragColor;

void main() {
  fragColor = texture(uTexture, fTex);
}
