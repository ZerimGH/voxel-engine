#version 330 core

out vec4 FragColor;

uniform float screenHeight;
uniform float cameraPitch;
uniform float cameraFOV;

void main() {
  vec3 topColor = vec3(0.404,0.561,0.765);
  vec3 bottomColor = vec3(0.98,0.569,0.341);

  float yNorm = gl_FragCoord.y / screenHeight;

  float horizon = 0.5 - 0.5 * tan(cameraPitch) / tan(radians(cameraFOV) * 0.5);

  float grad = 4.0;
  float t = clamp((yNorm - horizon) * grad, 0.0, 1.0);

  vec3 color = mix(bottomColor, topColor, t);
  FragColor = vec4(color, 1.0);
}
