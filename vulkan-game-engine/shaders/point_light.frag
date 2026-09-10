#version 450

// Offset 2D dal centro del billboard (compreso tra -1.0 e 1.0)
layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  vec4 ambientLightColor; // w is intensity
  vec3 lightPosition;
  vec4 lightColor;
} ubo;

void main() {
  // Calcola la distanza dal centro del billboard
  float dis = sqrt(dot(fragOffset, fragOffset));
  // Scarta (discard) i pixel oltre il raggio unitario per trasformare il quad quadrato in un cerchio perfetto
  if (dis >= 1.0) {
    discard;
  }
  outColor = vec4(ubo.lightColor.xyz, 1.0);
}