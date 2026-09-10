#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "lve_model.hpp"

// libs
#include <glm/gtc/matrix_transform.hpp>

// std
#include <memory>
#include <unordered_map>

namespace lve {

  // Componente per gestire le trasformazioni 3D: traslazione, scala e rotazione (angoli di Eulero).
  // Utilizza coordinate omogenee e una matrice 4x4 per combinare traslazione, rotazioni e scala in una sola operazione.
  struct TransformComponent {
    glm::vec3 translation{};             // Posizione 3D (x, y, z) nello spazio
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f }; // Fattore di scala lungo gli assi X, Y e Z (default: 1.0)
    glm::vec3 rotation{};                // Angoli di rotazione espressi in radianti attorno agli assi X, Y e Z

    // Calcola la matrice di trasformazione del modello 4x4 (traslazione * rotazione * scala)
    glm::mat4 mat4();
    // Calcola la matrice delle normali 3x3 (rotazione * inversa della scala) per trasformare correttamente
    // i vettori normali nello spazio mondo anche in presenza di scalatura non uniforme, ignorando la traslazione
    glm::mat3 normalMatrix();
  };

  // Rappresenta un'entità di gioco (Game Object).
  // Per ora utilizziamo un modello monolitico semplice in cui ogni game object possiede
  // direttamente i componenti (modello, colore, trasformazione).
  class LveGameObject {
  public:
    using id_t = unsigned int;
    // Mappa dei game object indicizzata per ID: permette ricerche rapide in tempo costante O(1)
    // e facilita le relazioni tra oggetti memorizzando semplicemente l'ID
    using Map = std::unordered_map<id_t, LveGameObject>;

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