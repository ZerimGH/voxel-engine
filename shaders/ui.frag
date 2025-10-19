#version 330 core

in vec2 fTex;

uniform sampler2D uTexture;
uniform sampler2DArray uTextureArray;
uniform bool uUseArray;
uniform int uIndex;

out vec4 fragColor;

void main() {
  if(!uUseArray) {
    fragColor = texture(uTexture, fTex);
  } else {
    fragColor = texture(uTextureArray, vec3(fTex, uIndex));
  }
}
