#version 330 core

out vec4 FragColor;

uniform float screenHeight;
uniform float cameraPitch;
uniform float cameraFOV;

void main() {
  vec3 topColor = vec3(0.45, 0.57, 0.895);
  vec3 bottomColor = vec3(0.603, 0.761, 0.965);

  float ndcY = (gl_FragCoord.y / screenHeight) * 2.0 - 1.0;

  float halfFovRad = radians(cameraFOV) * 0.5;
  float pixelAngleOffset = atan(ndcY * tan(halfFovRad));

  float worldAngle = cameraPitch + pixelAngleOffset;

  float gradientT = clamp(worldAngle * 5.0, 0.0, 1.0);

  vec3 color = mix(bottomColor, topColor, gradientT);
  FragColor = vec4(color, 1.0);
}
