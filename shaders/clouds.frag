#version 330 core

in vec2 fTex;
in vec2 fWorldPos;
out vec4 fragColor;

uniform sampler2D uTexture;
uniform vec3 uPlayerPos;
uniform float uRenderDistance;

void main() {
  vec4 col = texture(uTexture, fTex);

  float pRenderDistance = uRenderDistance * 8;
  
  float fog_near = pRenderDistance / 2.f;
  float fog_far = pRenderDistance;
  
  float dist = distance(fWorldPos, uPlayerPos.xz);
  dist = clamp(dist, fog_near, fog_far);
  
  float fFogFactor = (dist - fog_near) / (fog_far - fog_near);
  
  col.a = mix(col.a, 0.0f, fFogFactor);
  
  if(col.a < 0.005f) discard;
  
  fragColor = col;
}
