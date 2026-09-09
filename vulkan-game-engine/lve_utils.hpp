#pragma once

#include <functional>

namespace lve {
  // Funzione di utilità per combinare un numero arbitrario di valori hash in un unico seed.
  // Utilizza i "fold expressions" (funzionalità del C++17) per elaborare l'intero parameter pack.
  // Serve per calcolare l'hash di un'intera struttura (es. Vertex) combinando gli hash dei suoi campi,
  // così da poterla usare come chiave in un'std::unordered_map.
  // (Basato su: https://stackoverflow.com/a/57595105)
  template <typename T, typename... Rest>
  void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hashCombine(seed, rest), ...);
  };
} // namespace lve