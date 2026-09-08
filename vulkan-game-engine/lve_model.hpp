#pragma once

#include "lve_device.hpp"

// libs
// Forza GLM a utilizzare i radianti per gli angoli su qualsiasi piattaforma (evitando ambiguità con i gradi).
#define GLM_FORCE_RADIANS
// Indica a GLM di mappare l'intervallo di profondità su [0, 1] (standard Vulkan) anziché [-1, 1] (standard OpenGL).
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <vector>

namespace lve {
  // Gestisce il caricamento dei vertici dalla CPU, l'allocazione della memoria e il trasferimento su GPU (Vulkan buffer).
  class LveModel {
  public:
    // Rappresenta i singoli vertici e i relativi attributi (posizione 3D e colore RGB interleaved).
    struct Vertex {
      glm::vec3 position; // Posizione nello spazio tridimensionale (x, y, z)
      glm::vec3 color;

      // Descrive il binding (rate di avanzamento nello stream dei dati, stride in byte tra vertici successivi).
      static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
      // Descrive come interpretare ciascun attributo (location nello shader, binding di origine, formato dati, offset).
      static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    LveModel(LveDevice& lveDevice, const std::vector<Vertex>& vertices);
    ~LveModel();

    // Elimina costruttore di copia e operatore di assegnazione poiché la classe gestisce risorse Vulkan esplicite (buffer e memoria).
    LveModel(const LveModel&) = delete;
    LveModel& operator=(const LveModel&) = delete;

    // Registra nel command buffer il binding del vertex buffer.
    void bind(VkCommandBuffer commandBuffer);
    // Registra nel command buffer il comando di draw per tutti i vertici del modello.
    void draw(VkCommandBuffer commandBuffer);

  private:
    void createVertexBuffers(const std::vector<Vertex>& vertices);

    LveDevice& lveDevice;
    // In Vulkan, l'oggetto buffer e la memoria allocata ad esso associata sono gestiti separatamente dal programmatore.
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    uint32_t vertexCount;
  };
}
// namespace lve