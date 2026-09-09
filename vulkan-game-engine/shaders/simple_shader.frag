#version 450

// Riceve il colore interpolato dal vertex shader tramite lo stadio di rasterizzazione
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

// Blocco di Push Constants accessibile nel Fragment Shader (stessa definizione e layout di memoria del vertex shader)
layout(push_constant) uniform Push {
  mat4 transform; // Mantiene allineato il layout di memoria con il blocco push constants del vertex shader
  mat4 normalMatrix;
} push;

void main() {
  // Utilizza il colore per-vertice interpolato (fragColor) con canale Alpha = 1.0
  outColor = vec4(fragColor, 1.0);
}