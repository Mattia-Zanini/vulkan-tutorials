#include "lve_camera.hpp"

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
} // namespace lve