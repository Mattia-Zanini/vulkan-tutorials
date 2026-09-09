#pragma once

#include "lve_buffer.hpp"
#include "lve_device.hpp"

// libs
// Forza GLM a utilizzare i radianti per gli angoli su qualsiasi piattaforma (evitando ambiguità con i gradi).
#define GLM_FORCE_RADIANS
// Indica a GLM di mappare l'intervallo di profondità su [0, 1] (standard Vulkan) anziché [-1, 1] (standard OpenGL).
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <cstdint>
#include <memory>
#include <vector>

namespace lve {
  // Gestisce il caricamento dei vertici dalla CPU, l'allocazione della memoria e il trasferimento su GPU (Vulkan buffer).
  class LveModel {
  public:
    // Rappresenta i singoli vertici e i relativi attributi (posizione 3D e colore RGB interleaved).
    struct Vertex {
      glm::vec3 position{}; // Posizione nello spazio tridimensionale (x, y, z)
      glm::vec3 color{};
      glm::vec3 normal{}; // Normale del vertice, utilizzata per il calcolo dell'illuminazione
      glm::vec2 uv{};     // Coordinate texture 2D (spesso chiamate uvs) per mappare immagini sulla geometria

      // Descrive il binding (rate di avanzamento nello stream dei dati, stride in byte tra vertici successivi).
      static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
      // Descrive come interpretare ciascun attributo (location nello shader, binding di origine, formato dati, offset).
      static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

      // Sovraccarico dell'operatore di uguaglianza. È necessario per l'utilizzo in std::unordered_map
      // in modo da identificare e scartare vertici duplicati durante il caricamento.
      bool operator==(const Vertex& other) const {
        return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
      }
    };

    // Builder è un oggetto temporaneo di supporto utilizzato per memorizzare le informazioni
    // sui vertici e sugli indici prima che vengano copiati nella memoria del buffer del modello (sulla GPU).
    // Questo permette di evitare la duplicazione dei vertici (usando gli index buffers) e di aggiungere
    // futuri attributi senza appesantire la memoria.
    struct Builder {
      std::vector<Vertex> vertices{};
      std::vector<uint32_t> indices{};

      // Legge un file .obj (Wavefront) e popola vertices e indices utilizzando tinyobjloader.
      void loadModel(const std::string& filepath);
    };

    LveModel(LveDevice& lveDevice, const LveModel::Builder& builder);
    ~LveModel();

    // Elimina costruttore di copia e operatore di assegnazione poiché la classe gestisce risorse Vulkan esplicite (buffer e memoria).
    LveModel(const LveModel&) = delete;
    LveModel& operator=(const LveModel&) = delete;

    static std::unique_ptr<LveModel> createModelFromFile(LveDevice& device, const std::string& filepath);

    // Registra nel command buffer il binding del vertex buffer.
    void bind(VkCommandBuffer commandBuffer);
    // Registra nel command buffer il comando di draw per tutti i vertici del modello.
    void draw(VkCommandBuffer commandBuffer);

  private:
    void createVertexBuffers(const std::vector<Vertex>& vertices);
    void createIndexBuffers(const std::vector<uint32_t>& indices);

    LveDevice& lveDevice;

    // LveBuffer incapsula VkBuffer e VkDeviceMemory gestendone automaticamente il ciclo di vita (RAII).
    std::unique_ptr<LveBuffer> vertexBuffer;
    uint32_t vertexCount;

    // L'index buffer opzionale ci permette di specificare ogni vertice unico una sola volta
    // e di istruire la GPU su come combinarli in triangoli fornendo solo gli indici.
    bool hasIndexBuffer = false;
    std::unique_ptr<LveBuffer> indexBuffer;
    uint32_t indexCount;
  };
}
// namespace lve