#pragma once

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace lve {
  // LveCamera: gestisce la configurazione della camera e il calcolo della matrice di proiezione
  // (ortografica o prospettica) per mappare lo spazio vista/mondo nel canonical view volume di Vulkan.
  class LveCamera {
  public:
    // Configura una matrice di proiezione ortografica (parallela):
    // gli oggetti mantengono le stesse dimensioni indipendentemente dalla loro distanza dalla camera.
    // I parametri delimitano il volume di visualizzazione rettangolare (left, right, top, bottom, near, far).
    void setOrthographicProjection(float left, float right,
                                   float top, float bottom,
                                   float near, float far);

    // Configura una matrice di proiezione prospettica:
    // simula la visione dell'occhio umano introducendo il punto di fuga, in cui gli oggetti più lontani
    // appaiono progressivamente più piccoli.
    // fovy: campo visivo verticale espresso in radianti
    // aspect: rapporto tra larghezza e altezza (aspect ratio) del viewport
    // near e far: piani di taglio (clipping) vicino e lontano lungo l'asse Z
    void setPerspectiveProjection(float fovy, float aspect, float near, float far);

    // Restituisce un riferimento costante alla matrice di proiezione 4x4 corrente
    const glm::mat4& getProjection() const { return projectionMatrix; }

  private:
    // Matrice di proiezione 4x4 (inizializzata di default alla matrice identità)
    glm::mat4 projectionMatrix{ 1.f };
  };
} // namespace lve