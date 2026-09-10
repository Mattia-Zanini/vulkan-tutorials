#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "lve_model.hpp"
#include "lve_swap_chain.hpp"
#include "lve_texture.hpp"

// libs
#include <cstddef>
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

  // Componente per identificare un game object come point light (compatibile con PointLightSystem).
  // Posizione e raggio non vengono duplicati, ma riutilizzano i campi translation e scale.x di TransformComponent
  struct PointLightComponent {
    float lightIntensity = 1.0f;
  };

  struct GameObjectBufferData {
    glm::mat4 modelMatrix{ 1.f };
    glm::mat4 normalMatrix{ 1.f };
  };

  class LveGameObjectManager; // forward declare game object manager class

  // Rappresenta un'entità di gioco (Game Object).
  class LveGameObject {
  public:
    using id_t = unsigned int;
    using Map = std::unordered_map<id_t, LveGameObject>;

    LveGameObject(const LveGameObject&) = delete;
    LveGameObject& operator=(const LveGameObject&) = delete;
    LveGameObject(LveGameObject&&) = default;
    LveGameObject& operator=(LveGameObject&&) = delete;

    id_t getId() const { return id; }

    VkDescriptorBufferInfo getBufferInfo(int frameIndex);

    glm::vec3 color{};
    TransformComponent transform{};

    std::shared_ptr<LveModel> model{};
    std::shared_ptr<LveTexture> diffuseMap = nullptr;
    std::unique_ptr<PointLightComponent> pointLight = nullptr;

  private:
    LveGameObject(id_t objId, const LveGameObjectManager& manager);

    id_t id;
    const LveGameObjectManager& gameObjectManger;

    friend class LveGameObjectManager;
  };

  class LveGameObjectManager {
  public:
    static constexpr int MAX_GAME_OBJECTS = 1000;

    LveGameObjectManager(LveDevice& device);
    LveGameObjectManager(const LveGameObjectManager&) = delete;
    LveGameObjectManager& operator=(const LveGameObjectManager&) = delete;
    LveGameObjectManager(LveGameObjectManager&&) = delete;
    LveGameObjectManager& operator=(LveGameObjectManager&&) = delete;

    LveGameObject& createGameObject() {
      assert(currentId < MAX_GAME_OBJECTS && "Max game object count exceeded!");
      auto gameObject = LveGameObject{ currentId++, *this };
      auto gameObjectId = gameObject.getId();
      gameObject.diffuseMap = textureDefault;
      gameObjects.emplace(gameObjectId, std::move(gameObject));
      return gameObjects.at(gameObjectId);
    }

    LveGameObject& makePointLight(
      float intensity = 10.f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.f));

    VkDescriptorBufferInfo getBufferInfoForGameObject(int frameIndex, id_t gameObjectId) const {
      return uboBuffers[frameIndex]->descriptorInfoForIndex(gameObjectId);
    }

    void updateBuffer(int frameIndex);

    LveGameObject::Map gameObjects{};
    std::vector<std::unique_ptr<LveBuffer>> uboBuffers{ LveSwapChain::MAX_FRAMES_IN_FLIGHT };

  private:
    id_t currentId = 0;
    std::shared_ptr<LveTexture> textureDefault;
  };

} // namespace lve