#version 450

// Attributo del vertice: 'in' specifica che il valore proviene dal vertex buffer (anziché essere hardcoded).
// 'layout(location = 0)' associa questa variabile alla descrizione dell'attributo (location 0) configurata nella pipeline.
layout(location = 0) in vec2 position;

void main() {
  // Lo shader viene eseguito una volta per ogni vertice; a ogni invocazione 'position' riceve le coordinate x, y dal buffer.
  gl_Position = vec4(position, 0.0, 1.0);
}