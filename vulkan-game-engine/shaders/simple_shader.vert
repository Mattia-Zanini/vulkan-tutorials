#version 450

// Attributi del vertice (input dal vertex buffer):
// 'layout(location = 0)' associa la posizione 2D (vec2).
layout(location = 0) in vec2 position;
// 'layout(location = 1)' associa il colore RGB (vec3).
layout(location = 1) in vec3 color;

// Blocco di Push Constants accessibile nel Vertex Shader
layout(push_constant) uniform Push {
  vec2 offset;
  vec3 color;
}
push;

void main() {
  // Trasla la posizione del vertice sommando l'offset condiviso passato tramite push constant
  gl_Position = vec4(position + push.offset, 0.0, 1.0);
}