#version 450

// Offset 2D dal centro del billboard (compreso tra -1.0 e 1.0)
layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

// Dati di una singola point light memorizzati nell'UBO (allineamento std140)
struct PointLight {
  vec4 position; // ignore w
  vec4 color;  // w is intensity
};

// Uniform Buffer Object globale (stessa struttura per tutti gli shader dell'engine)
layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView; // Matrice di vista inversa per estrarre la posizione della camera nello spazio mondo
  vec4 ambientLightColor; // w is intensity
  PointLight pointLights[10];
  int numLights;
} ubo;

// Dati push constant passati per ciascuna point light disegnata
layout(push_constant) uniform Push {
  vec4 position;
  vec4 color;
  float radius;
} push;

void main() {
  // Calcola la distanza dal centro del billboard
  float dis = sqrt(dot(fragOffset, fragOffset));
  // Scarta (discard) i pixel oltre il raggio unitario per trasformare il quad quadrato in un cerchio perfetto
  if (dis >= 1.0) {
    discard;
  }

  // Assegna il colore RGB della specifica point light passato tramite push constants
  outColor = vec4(push.color.xyz, 1.0);
}