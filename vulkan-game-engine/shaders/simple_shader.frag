#version 450

// Riceve attributi interpolati dal vertex shader tramite lo stadio di rasterizzazione
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;    // Posizione del frammento nello spazio mondo
layout(location = 2) in vec3 fragNormalWorld; // Normale del frammento nello spazio mondo

layout(location = 0) out vec4 outColor;

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

// Blocco di Push Constants accessibile nel Fragment Shader (stessa definizione e layout di memoria del vertex shader)
layout(push_constant) uniform Push {
  mat4 modelMatrix; // Mantiene allineato il layout di memoria con il blocco push constants del vertex shader
  mat4 normalMatrix;
} push;

void main() {
  // Inizializza la luce totale con il contributo della luce ambientale
  vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
  // Calcola la normale del frammento una sola volta all'esterno del ciclo per evitare ricalcoli ridondanti
  vec3 surfaceNormal = normalize(fragNormalWorld);

  // Itera su tutte le point light attive per accumulare il contributo diffuso totale
  for (int i = 0; i < ubo.numLights; i++) {
    PointLight light = ubo.pointLights[i];

    // Calcola il vettore direzione dal frammento verso la point light corrente nello spazio mondo
    vec3 directionToLight = light.position.xyz - fragPosWorld;

    // Attenuazione secondo la legge dell'inverso del quadrato della distanza (1 / d^2).
    // Il prodotto scalare del vettore con se stesso calcola il quadrato della distanza in modo efficiente (e va fatto prima di normalizzare)
    float attenuation = 1.0 / dot(directionToLight, directionToLight);
    // Coseno dell'angolo di incidenza tra normale della superficie e direzione della luce (modello di Lambert)
    float cosAngIncidence = max(dot(surfaceNormal, normalize(directionToLight)), 0);
    // Intensità della luce scalata per intensità base (in color.w) e attenuazione con la distanza
    vec3 intensity = light.color.xyz * light.color.w * attenuation;

    diffuseLight += intensity * cosAngIncidence;
  }

  // Utilizza il colore per-vertice interpolato (fragColor) con canale Alpha = 1.0
  outColor = vec4(diffuseLight * fragColor, 1.0);
}