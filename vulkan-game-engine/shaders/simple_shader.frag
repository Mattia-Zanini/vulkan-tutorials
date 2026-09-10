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
  mat4 invView; // Matrice di vista inversa per estrarre la posizione della camera nello spazio mondo
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
  vec3 specularLight = vec3(0.0); // Accumulatore per il contributo di luce speculare (modello Blinn-Phong)
  // Calcola la normale del frammento una sola volta all'esterno del ciclo per evitare ricalcoli ridondanti
  vec3 surfaceNormal = normalize(fragNormalWorld);

  // Estrae la posizione della telecamera nello spazio mondo dall'ultima colonna della matrice di vista inversa
  vec3 cameraPosWorld = ubo.invView[3].xyz;
  // Vettore direzione unitario dalla superficie verso la telecamera (osservatore)
  vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

  // Itera su tutte le point light attive per accumulare il contributo diffuso totale
  for (int i = 0; i < ubo.numLights; i++) {
    PointLight light = ubo.pointLights[i];

    // Calcola il vettore direzione dal frammento verso la point light corrente nello spazio mondo
    vec3 directionToLight = light.position.xyz - fragPosWorld;

    // Attenuazione secondo la legge dell'inverso del quadrato della distanza (1 / d^2).
    // Il prodotto scalare del vettore con se stesso calcola il quadrato della distanza in modo efficiente (e va fatto prima di normalizzare)
    float attenuation = 1.0 / dot(directionToLight, directionToLight);
    directionToLight = normalize(directionToLight);

    // Coseno dell'angolo di incidenza tra normale della superficie e direzione della luce (modello di Lambert)
    float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
    // Intensità della luce scalata per intensità base (in color.w) e attenuazione con la distanza
    vec3 intensity = light.color.xyz * light.color.w * attenuation;

    diffuseLight += intensity * cosAngIncidence;

    // Illuminazione speculare (Blinn-Phong): calcola il vettore a metà angolo tra la direzione della luce e della vista.
    // L'angolo tra halfAngle e la normale è sempre < 90°, superando il limite del modello di Phong tradizionale
    vec3 halfAngle = normalize(directionToLight + viewDirection);
    float blinnTerm = dot(surfaceNormal, halfAngle);
    // Clampa a zero per ignorare i casi in cui osservatore e luce si trovano su lati opposti della superficie
    blinnTerm = clamp(blinnTerm, 0, 1);
    // Esponente speculare: valori più alti schiacciano i valori bassi a zero, producendo un riflesso più nitido e compatto
    blinnTerm = pow(blinnTerm, 512.0); // higher values -> sharper highlight
    specularLight += intensity * blinnTerm;
  }

  // Combina illuminazione diffusa e speculare modulandole con il colore del materiale (fragColor)
  outColor = vec4(diffuseLight * fragColor + specularLight * fragColor, 1.0);
}