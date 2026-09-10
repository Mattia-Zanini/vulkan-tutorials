#pragma once

// libs
#include "glm/ext/vector_float3.hpp"
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

    // La "camera transformation" (o "view transformation") permette di visualizzare il mondo
    // da qualsiasi punto di vista. Trasla il mondo in modo che la camera sia all'origine
    // e lo ruota per allinearlo agli assi canonici.

    // Imposta la vista specificando direttamente la direzione in cui guarda la camera.
    // Costruisce una base ortonormale (u, v, w) per la rotazione.
    void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = { 0.f, -1.f, 0.f });

    // Imposta la vista puntando la camera verso un punto specifico (target).
    void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = { 0.f, -1.f, 0.f });

    // Imposta la vista utilizzando gli angoli di Eulero (Tait-Bryan) in ordine Y, X, Z.
    // Sfrutta la dualità tra rotazioni intrinseche ed estrinseche, applicando le
    // trasformazioni inverse per spostare il mondo rispetto alla camera.
    void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

    // Restituisce un riferimento costante alla matrice di proiezione 4x4 corrente
    const glm::mat4& getProjection() const { return projectionMatrix; }
    // Restituisce la matrice di vista (camera transform) corrente
    const glm::mat4& getView() const { return viewMatrix; }
    // Restituisce la matrice di vista inversa (permette di estrarre la posizione della camera nello spazio mondo)
    const glm::mat4& getInverseView() const { return inverseViewMatrix; }

  private:
    // Matrice di proiezione 4x4 (inizializzata di default alla matrice identità)
    glm::mat4 projectionMatrix{ 1.f };
    // Matrice di vista 4x4 per la trasformazione della camera
    glm::mat4 viewMatrix{ 1.f };
    // Matrice di vista inversa (trasforma dallo spazio camera allo spazio mondo; equivalente al transform del viewer)
    glm::mat4 inverseViewMatrix{ 1.f };
  };
} // namespace lve