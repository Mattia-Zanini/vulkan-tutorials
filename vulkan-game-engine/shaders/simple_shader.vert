#version 450

// Attributi del vertice (input dal vertex buffer):
// 'layout(location = 0)' associa la posizione 3D (vec3: x, y, z).
layout(location = 0) in vec3 position;
// 'layout(location = 1)' associa il colore RGB per vertice (vec3).
layout(location = 1) in vec3 color;
// 'layout(location = 2)' associa la normale del vertice per il calcolo dell'illuminazione (vec3).
layout(location = 2) in vec3 normal;
// 'layout(location = 3)' associa le coordinate texture UV 2D (vec2).
layout(location = 3) in vec2 uv;

// Output verso il fragment shader per il calcolo dell'illuminazione per-fragment
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;    // Posizione del vertice nello spazio mondo (interpolata per frammento)
layout(location = 2) out vec3 fragNormalWorld; // Normale del vertice nello spazio mondo (interpolata per frammento)

// Dati di una singola point light memorizzati nell'UBO (allineamento std140)
struct PointLight {
  vec4 position; // ignore w
  vec4 color;  // w is intensity
};

// Uniform Buffer Object globale (stessa struttura per tutti gli shader dell'engine)
layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  vec4 ambientLightColor; // w is intensity
  PointLight pointLights[10];
  int numLights;
} ubo;

// Blocco di Push Constants accessibile nel Vertex Shader
layout(push_constant) uniform Push {
  mat4 modelMatrix; // Matrice di trasformazione specifica del singolo modello
  mat4 normalMatrix;
} push;

void main() {
  // Trasforma la posizione del vertice nello spazio mondo (necessario poiché la posizione della point light è in world space)
  vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

  // Trasforma la posizione nello spazio di proiezione/clip applicando separatamente view e projection (l'ordine di moltiplicazione è critico)
  gl_Position = ubo.projection * ubo.view * positionWorld;

  // Trasforma la normale nello spazio mondo estraendo la sottomatrice 3x3 dalla normalMatrix (passata come mat4 per allineamento)
  fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
  // Inoltra posizione nello spazio mondo e colore del vertice allo stadio di rasterizzazione
  fragPosWorld = positionWorld.xyz;
  fragColor = color;
}