#version 450

// Attributi del vertice (input dal vertex buffer):
// 'layout(location = 0)' associa la posizione 2D (vec2).
layout(location = 0) in vec2 position;
// 'layout(location = 1)' associa il colore RGB (vec3).
layout(location = 1) in vec3 color;

// Attributo di output verso il fragment shader:
// 'out' specifica un valore calcolato per-vertice che verrà interpolato linearmente dallo stadio di rasterizzazione
// tramite coordinate baricentriche su ciascun frammento coperto dal triangolo.
// Nota: la numerazione delle location di output è indipendente da quella di input.
layout(location = 0) out vec3 fragColor;

void main() {
  // Lo shader viene eseguito una volta per ogni vertice; a ogni invocazione 'position' riceve le coordinate x, y dal buffer.
  gl_Position = vec4(position, 0.0, 1.0);
  // Passa il colore del vertice all'output per essere interpolato
  fragColor = color;
}