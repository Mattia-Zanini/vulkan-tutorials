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

// Output verso il fragment shader per il colore interpolato (per-vertex coloring)
layout(location = 0) out vec3 fragColor;

// Uniform Buffer Object globale accessibile tramite descriptor set 0 al binding 0
layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projectionViewMatrix;
  vec4 ambientLightColor; // RGB = colore, A = intensità
  vec3 lightPosition;
  vec4 lightColor; // RGB = colore, A = intensità
} ubo;

// Blocco di Push Constants accessibile nel Vertex Shader
layout(push_constant) uniform Push {
  mat4 modelMatrix; // Matrice di trasformazione specifica del singolo modello
  mat4 normalMatrix;
} push;

void main() {
  // Trasforma la posizione del vertice nello spazio mondo (necessario poiché la posizione della point light è in world space)
  vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

  // Trasforma la posizione nello spazio di proiezione/clip
  gl_Position = ubo.projectionViewMatrix * positionWorld;

  // Trasforma la normale nello spazio mondo estraendo la sottomatrice 3x3 dalla normalMatrix (passata come mat4 per allineamento).
  // La normalizzazione garantisce che il vettore risultante sia di lunghezza unitaria.
  vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * normal);

  // Calcola il vettore direzione dal vertice verso la point light nell world space
  vec3 directionToLight = ubo.lightPosition - positionWorld.xyz;

  // Attenuazione secondo la legge dell'inverso del quadrato della distanza (1 / d^2).
  // Il prodotto scalare del vettore con se stesso calcola il quadrato della distanza in modo efficiente (e va fatto prima di normalizzare)
  float attenuation = 1.0 / dot(directionToLight, directionToLight);

  // Regolo le luci in base alle loro intensità base e all'attenuazione
  vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
  vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

  // Modello di illuminazione diffusa (Lambertiano): proporzionale al coseno dell'angolo tra la normale e la direzione normalizzata della luce
  vec3 diffuseLight = lightColor * max(dot(normalWorldSpace, normalize(directionToLight)), 0);

  // Modula il colore del vertice moltiplicandolo per la somma di luce diffusa e ambientale
  fragColor = (diffuseLight + ambientLight) * color;
}