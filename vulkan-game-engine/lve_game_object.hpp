#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "lve_model.hpp"

// libs
#include <glm/gtc/matrix_transform.hpp>

// std
#include <memory>

namespace lve {

  // Componente per gestire le trasformazioni 3D: traslazione, scala e rotazione (angoli di Eulero).
  // Utilizza coordinate omogenee e una matrice 4x4 per combinare traslazione, rotazioni e scala in una sola operazione.
  struct TransformComponent {
    glm::vec3 translation{};             // Posizione 3D (x, y, z) nello spazio
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f }; // Fattore di scala lungo gli assi X, Y e Z (default: 1.0)
    glm::vec3 rotation{};                // Angoli di rotazione espressi in radianti attorno agli assi X, Y e Z

    // Calcola la matrice di trasformazione affine 4x4 combinata (Translate * Ry * Rx * Rz * Scale).
    // Le rotazioni utilizzano la convenzione estrinseca degli angoli di Tait-Bryan con ordine Y(1), X(2), Z(3)
    // (equivalente all'ordine intrinseco inverso Z-X-Y: roll, pitch, yaw).
    // La formula espansa algebricamente evita moltiplicazioni ridondanti e chiamate multiple a glm::rotate.
    // I costruttori delle matrici GLM accettano vettori colonna:
    // le prime 3 colonne codificano scala e rotazione (con 4a componente 0.0),
    // mentre la 4a colonna codifica la traslazione con coordinata omogenea w = 1.0.
    // Riferimento matematico: https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
    glm::mat4 mat4() {
      const float c3 = glm::cos(rotation.z);
      const float s3 = glm::sin(rotation.z);
      const float c2 = glm::cos(rotation.x);
      const float s2 = glm::sin(rotation.x);
      const float c1 = glm::cos(rotation.y);
      const float s1 = glm::sin(rotation.y);
      return glm::mat4{
        {
          scale.x * (c1 * c3 + s1 * s2 * s3),
          scale.x * (c2 * s3),
          scale.x * (c1 * s2 * s3 - c3 * s1),
          0.0f,
        },
        {
          scale.y * (c3 * s1 * s2 - c1 * s3),
          scale.y * (c2 * c3),
          scale.y * (c1 * c3 * s2 + s1 * s3),
          0.0f,
        },
        {
          scale.z * (c2 * s1),
          scale.z * (-s2),
          scale.z * (c1 * c2),
          0.0f,
        },
        { translation.x, translation.y, translation.z, 1.0f }
      };
    }
  };

  // Rappresenta un'entità di gioco (Game Object).
  // Per ora utilizziamo un modello monolitico semplice in cui ogni game object possiede
  // direttamente i componenti (modello, colore, trasformazione).
  class LveGameObject {
  public:
    using id_t = unsigned int;

    // Factory method per creare game object con identificativo ID univoco incrementale
    static LveGameObject createGameObject() {
      static id_t currentId = 0;
      return LveGameObject{ currentId++ };
    }

    // I game object possiedono un ID univoco: impediamo la copia accidentale,
    // consentendo invece lo spostamento (move semantics).
    LveGameObject(const LveGameObject&) = delete;
    LveGameObject& operator=(const LveGameObject&) = delete;
    LveGameObject(LveGameObject&&) = default;
    LveGameObject& operator=(LveGameObject&&) = default;

    // Restituisce l'ID univoco dell'oggetto
    id_t getid() const { return id; }

    // Riferimento condiviso al modello: più game object possono condividere lo stesso vertex buffer allocato sulla GPU
    std::shared_ptr<LveModel> model{};
    glm::vec3 color{};
    TransformComponent transform{};

  private:
    // Costruttore privato: la creazione è consentita esclusivamente tramite factory method createGameObject()
    LveGameObject(id_t objId) : id{ objId } {}

    id_t id;
  };
} // namespace lve