#version 450

// Offset dei 6 vertici (2 triangoli) nello spazio camera per formare un quad (quadrato) centrato sulla point light
const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

// Offset interpolato passato al fragment shader (varia tra -1 e 1) per calcolare il cerchio del billboard
layout (location = 0) out vec2 fragOffset;

// Uniform Buffer Object globale (stessa struttura per tutti gli shader dell'engine)
layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  vec4 ambientLightColor; // w is intensity
  vec3 lightPosition;
  vec4 lightColor;
} ubo;

const float LIGHT_RADIUS = 0.05; // Raggio del billboard della point light

void main() {
  fragOffset = OFFSETS[gl_VertexIndex];

  // Estrae i vettori Right e Up della camera nello spazio mondo direttamente dalle righe della matrice di vista
  vec3 cameraRightWorld = {ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]};
  vec3 cameraUpWorld = {ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]};

  // Calcola la posizione del vertice nello spazio mondo (screen-aligned billboard):
  // parte dal centro della luce e sposta il vertice lungo i vettori Up e Right della camera
  vec3 positionWorld = ubo.lightPosition.xyz
    + LIGHT_RADIUS * fragOffset.x * cameraRightWorld
    + LIGHT_RADIUS * fragOffset.y * cameraUpWorld;

  // Trasforma la posizione nel piano di proiezione (clip space)
  gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);
}