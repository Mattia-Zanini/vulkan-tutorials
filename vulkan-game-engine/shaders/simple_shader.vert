#version 450

// Attributi del vertice (input dal vertex buffer):
// 'layout(location = 0)' associa la posizione 2D (vec2).
layout(location = 0) in vec2 position;
// 'layout(location = 1)' associa il colore RGB (vec3).
layout(location = 1) in vec3 color;

// Blocco di Push Constants accessibile nel Vertex Shader
layout(push_constant) uniform Push {
  mat2 transform; // Matrice di trasformazione 2x2 (combina rotazione e scala)
  vec2 offset;
  vec3 color;
}
push;

void main() {
  // Applica la trasformazione lineare moltiplicando la matrice 2x2 per la posizione locale del vertice,
  // poi trasla il risultato sommando l'offset (vettore traslazione).
  // Nota: l'ordine della moltiplicazione matrice-vettore non è commutativo; 'transform * position' valuta le colonne per le componenti del vettore.
  gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0);
}