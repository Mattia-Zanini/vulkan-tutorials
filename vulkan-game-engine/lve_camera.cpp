#include "lve_camera.hpp"
#include "glm/ext/vector_float3.hpp"

// std
#include <cassert>
#include <limits>

namespace lve {
  // Costruisce la matrice di proiezione ortografica mappando il volume [left, right] x [top, bottom] x [near, far]
  // direttamente nel canonical viewing volume di Vulkan: x in [-1, 1], y in [-1, 1], z in [0, 1].
  // Nota: l'asse Y di Vulkan punta verso il basso, pertanto il fattore di scala per Y tiene conto di (bottom - top).
  void LveCamera::setOrthographicProjection(
    float left, float right, float top, float bottom, float near, float far) {
    projectionMatrix = glm::mat4{ 1.0f };
    projectionMatrix[0][0] = 2.f / (right - left);
    projectionMatrix[1][1] = 2.f / (bottom - top);
    projectionMatrix[2][2] = 1.f / (far - near);
    projectionMatrix[3][0] = -(right + left) / (right - left);
    projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
    projectionMatrix[3][2] = -near / (far - near);
  }

  // Costruisce la matrice di proiezione prospettica per Vulkan:
  // - Scala x in funzione dell'aspect ratio e del campo visivo fovy (1 / (aspect * tan(fovy / 2)))
  // - Scala y in funzione di fovy (1 / tan(fovy / 2))
  // - Mappa l'intervallo di profondità Z da [near, far] nell'intervallo [0, 1] di Vulkan
  // - Salva la coordinata Z in W (projectionMatrix[2][3] = 1.0f) per consentire la prospettiva division (divisione per w nella pipeline)
  void LveCamera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    const float tanHalfFovy = tan(fovy / 2.f);
    projectionMatrix = glm::mat4{ 0.0f };
    projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
    projectionMatrix[1][1] = 1.f / (tanHalfFovy);
    projectionMatrix[2][2] = far / (far - near);
    projectionMatrix[2][3] = 1.f;
    projectionMatrix[3][2] = -(far * near) / (far - near);
  }

  void LveCamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
    // Costruisce una base ortonormale: w è in direzione della telecamera,
    // u punta a destra e v punta verso l'alto (o il basso, a seconda della convenzione)
    const glm::vec3 w{ glm::normalize(direction) };
    const glm::vec3 u{ glm::normalize(glm::cross(w, up)) };
    const glm::vec3 v{ glm::cross(w, u) };

    viewMatrix = glm::mat4{ 1.f };
    // Le prime tre colonne (e righe) memorizzano la rotazione trasposta (inversa)
    viewMatrix[0][0] = u.x;
    viewMatrix[1][0] = u.y;
    viewMatrix[2][0] = u.z;
    viewMatrix[0][1] = v.x;
    viewMatrix[1][1] = v.y;
    viewMatrix[2][1] = v.z;
    viewMatrix[0][2] = w.x;
    viewMatrix[1][2] = w.y;
    viewMatrix[2][2] = w.z;
    // L'ultima riga calcola la traslazione inversa (-posizione) ruotata usando il prodotto scalare
    viewMatrix[3][0] = -glm::dot(u, position);
    viewMatrix[3][1] = -glm::dot(v, position);
    viewMatrix[3][2] = -glm::dot(w, position);
  }

  void LveCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
    // Calcola il vettore direzione dal punto della telecamera verso il bersaglio
    glm::vec3 direction = target - position;

    assert(glm::dot(direction, direction) > std::numeric_limits<float>::epsilon() && "Can't have a null vector as target-camera direction");

    setViewDirection(position, direction, up);
  }

  void LveCamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
    // Calcola il seno e il coseno per gli angoli di Eulero (Tait-Bryan: Y, X, Z)
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);

    // Costruisce i vettori base dalla matrice di rotazione inversa
    const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
    const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
    const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };

    viewMatrix = glm::mat4{ 1.f };
    viewMatrix[0][0] = u.x;
    viewMatrix[1][0] = u.y;
    viewMatrix[2][0] = u.z;
    viewMatrix[0][1] = v.x;
    viewMatrix[1][1] = v.y;
    viewMatrix[2][1] = v.z;
    viewMatrix[0][2] = w.x;
    viewMatrix[1][2] = w.y;
    viewMatrix[2][2] = w.z;
    // Applica la traslazione inversa (sposta il mondo opposto alla posizione della camera)
    viewMatrix[3][0] = -glm::dot(u, position);
    viewMatrix[3][1] = -glm::dot(v, position);
    viewMatrix[3][2] = -glm::dot(w, position);
  }

} // namespace lve