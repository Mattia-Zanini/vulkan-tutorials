#version 450

// Riceve il colore interpolato dallo stadio di rasterizzazione (corrisponde a layout(location = 0) out nello vertex shader).
// Ciò che conta è che location e tipo di dato coincidano (il nome della variabile può differire).
layout(location = 0) out vec4 outColor;

// Blocco di Push Constants accessibile nel Fragment Shader (stessa definizione e layout di memoria del vertex shader)
layout(push_constant) uniform Push {
  vec2 offset;
  vec3 color;
}
push;

void main() {
  // Assegna il colore fornito direttamente tramite push constant con canale Alpha = 1.0
  outColor = vec4(push.color, 1.0);
}