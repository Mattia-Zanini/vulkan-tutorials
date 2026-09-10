#version 450

// Riceve attributi interpolati dal vertex shader tramite lo stadio di rasterizzazione
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;    // Posizione del frammento nello spazio mondo
layout(location = 2) in vec3 fragNormalWorld; // Normale del frammento nello spazio mondo

layout(location = 0) out vec4 outColor;

// Uniform Buffer Object globale accessibile tramite descriptor set 0 al binding 0
layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projectionViewMatrix;
  vec4 ambientLightColor; // RGB = colore, A = intensità
  vec3 lightPosition;
  vec4 lightColor; // RGB = colore, A = intensità
} ubo;

// Blocco di Push Constants accessibile nel Fragment Shader (stessa definizione e layout di memoria del vertex shader)
layout(push_constant) uniform Push {
  mat4 modelMatrix; // Mantiene allineato il layout di memoria con il blocco push constants del vertex shader
  mat4 normalMatrix;
} push;

void main() {
  // Calcola il vettore direzione dal vertice verso la point light nell world space
  vec3 directionToLight = ubo.lightPosition - fragPosWorld;

  // Attenuazione secondo la legge dell'inverso del quadrato della distanza (1 / d^2).
  // Il prodotto scalare del vettore con se stesso calcola il quadrato della distanza in modo efficiente (e va fatto prima di normalizzare)
  float attenuation = 1.0 / dot(directionToLight, directionToLight);

  // Regolo le luci in base alle loro intensità base e all'attenuazione
  vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
  vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

  // Modello di illuminazione diffusa (Lambertiano): proporzionale al coseno dell'angolo tra la normale e la direzione della luce.
  // È indispensabile normalizzare nuovamente fragNormalWorld poiché l'interpolazione lineare tra i vertici non ne preserva la lunghezza unitaria
  vec3 diffuseLight = lightColor * max(dot(normalize(fragNormalWorld), normalize(directionToLight)), 0);

  // Utilizza il colore per-vertice interpolato (fragColor) con canale Alpha = 1.0
  outColor = vec4((diffuseLight + ambientLight) * fragColor, 1.0);
}