#pragma once

#include "glm/ext/matrix_float2x2.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/trigonometric.hpp"
#include "lve_model.hpp"

// std
#include <memory>

namespace lve {

  // Componente per gestire le trasformazioni 2D: traslazione, scala e rotazione.
  // Utilizza una matrice 2x2 per combinare scala e rotazione (trasformazioni lineari).
  struct Transform2dComponent {
    glm::vec2 translation{};       // Posizione 2D (offset)
    glm::vec2 scale{ 1.0f, 1.0f }; // Fattore di scala su asse X e Y (default: 1.0, nessuna alterazione di dimensione)
    float rotation;                // Angolo di rotazione espresso in radianti

    // Costruisce e restituisce la matrice di trasformazione lineare 2x2 (rotazione * scala).
    // In GLSL e GLM, i costruttori delle matrici accettano vettori colonna:
    // ogni colonna rappresenta la nuova destinazione dei vettori della base standard:
    // colonna 0 = vettore i (1, 0), colonna 1 = vettore j (0, 1).
    glm::mat2 mat2() {
      const float s = glm::sin(rotation);
      const float c = glm::cos(rotation);
      // Matrice di rotazione 2D: definita per colonne {colonna0, colonna1}.
      // In coordinate normalizzate Vulkan con asse Y rivolto verso il basso,
      // questo produce visivamente una rotazione in senso orario.
      glm::mat2 rotMatrix{ { c, s }, { -s, c } };

      // Matrice di scala 2D: scala lungo i rispettivi assi principali
      glm::mat2 scaleMat{ { scale.x, 0.0f }, { 0.0f, scale.y } };

      // La moltiplicazione tra matrici non è commutativa:
      // Moltiplicando rotMatrix * scaleMat applichiamo prima la scala e poi la rotazione (ordine di valutazione da destra a sinistra).
      return rotMatrix * scaleMat;
    };
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
    const id_t getid() { return id; }

    // Riferimento condiviso al modello: più game object possono condividere lo stesso vertex buffer allocato sulla GPU
    std::shared_ptr<LveModel> model{};
    glm::vec3 color{};
    Transform2dComponent transform2d{};

  private:
    // Costruttore privato: la creazione è consentita esclusivamente tramite factory method createGameObject()
    LveGameObject(id_t objId) : id{ objId } {}

    id_t id;
  };
} // namespace lve