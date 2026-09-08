#version 450

// Attributi del vertice (input dal vertex buffer):
// 'layout(location = 0)' associa la posizione 3D (vec3: x, y, z).
layout(location = 0) in vec3 position;
// 'layout(location = 1)' associa il colore RGB per vertice (vec3).
layout(location = 1) in vec3 color;

// Output verso il fragment shader per il colore interpolato (per-vertex coloring)
layout(location = 0) out vec3 fragColor;

// Blocco di Push Constants accessibile nel Vertex Shader
layout(push_constant) uniform Push {
  mat4 transform; // Matrice di trasformazione affine 4x4 (combina scala, rotazione ed offset/traslazione)
  vec3 color;
}
push;

void main() {
  // Applica la trasformazione affine moltiplicando la matrice 4x4 per la posizione espressa in coordinate omogenee.
  // La quarta coordinata omogenea (w = 1.0) permette di applicare la componente di traslazione memorizzata nella matrice;
  // per un vettore direzione, si utilizzerebbe invece w = 0.0 per ignorare la traslazione.
  gl_Position = push.transform * vec4(position, 1.0);
  // Inoltra il colore del vertice al rasterizzatore per l'interpolazione baricentrica
  fragColor = color;
}