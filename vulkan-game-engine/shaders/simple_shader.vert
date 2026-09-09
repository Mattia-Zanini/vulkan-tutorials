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

// Blocco di Push Constants accessibile nel Vertex Shader
layout(push_constant) uniform Push {
  mat4 transform; // projection * view * model
  mat4 normalMatrix;
} push;

// Direzione della sorgente di luce nello spazio mondo, normalizzata a lunghezza unitaria.
// Simula una luce direzionale (come il sole) posta a distanza infinita, dove i raggi sono paralleli per tutti i vertici.
const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, -3.0, -1.0));

// Luce ambientale: approssimazione dell'illuminazione indiretta (luce rimbalzata nell'ambiente).
// Garantisce che anche le parti del modello non direttamente rivolte verso la sorgente luminosa non siano completamente nere.
const float AMBIENT = 0.02;

void main() {
  // Applica la trasformazione affine moltiplicando la matrice 4x4 per la posizione espressa in coordinate omogenee.
  // La quarta coordinata omogenea (w = 1.0) permette di applicare la componente di traslazione memorizzata nella matrice;
  // per un vettore direzione, si utilizzerebbe invece w = 0.0 per ignorare la traslazione.
  gl_Position = push.transform * vec4(position, 1.0);

  // Trasforma la normale nello spazio mondo estraendo la sottomatrice 3x3 dalla normalMatrix (passata come mat4 per allineamento).
  // La normalizzazione garantisce che il vettore risultante sia di lunghezza unitaria.
  vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * normal);

  // Modello di illuminazione diffusa (Lambertiano): l'intensità luminosa è proporzionale al coseno dell'angolo
  // tra la normale e la direzione della luce (calcolato tramite prodotto scalare dot product).
  // La funzione max con 0 assicura che le superfici rivolte in direzione opposta alla luce abbiano intensità nulla.
  float lighIntensity = AMBIENT + max(dot(normalWorldSpace, DIRECTION_TO_LIGHT), 0);

  // Modula il colore del vertice moltiplicandolo per l'intensità luminosa calcolata
  fragColor = lighIntensity * color;
}