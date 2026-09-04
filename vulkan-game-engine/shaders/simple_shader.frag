#version 450

// Riceve il colore interpolato dallo stadio di rasterizzazione (corrisponde a layout(location = 0) out nello vertex shader).
// Ciò che conta è che location e tipo di dato coincidano (il nome della variabile può differire).
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
  // Assegna il colore interpolato con canale Alpha a 1.0 al pixel corrente del framebuffer
  outColor = vec4(fragColor, 1.0);
}