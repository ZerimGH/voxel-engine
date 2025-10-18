#version 330 core

out vec4 fragColor;

in vec2 fTex;
in float brightness;
flat in int texture_index;
in float fFogFactor;

uniform sampler2DArray uTexture;

void main() {
  vec4 color = texture(uTexture, vec3(fTex, texture_index));
  vec3 color_rgb = color.rgb;
  vec4 pcolor = vec4(color_rgb * brightness, color.a);
  vec4 fogColor = vec4(0.603,0.761,0.965, 1.f);
  fragColor = mix(pcolor, fogColor, fFogFactor);
}
