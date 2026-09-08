#include "lve_model.hpp"
#include "vulkan/vulkan_core.h"

// std
#include <cassert>
#include <cstring>

namespace lve {
  LveModel::LveModel(LveDevice& device, const std::vector<Vertex>& vertices) : lveDevice{ device } {
    createVertexBuffers(vertices);
  }

  LveModel::~LveModel() {
    // Rilascio esplicito del buffer e della porzione di memoria allocata sulla GPU
    vkDestroyBuffer(lveDevice.device(), vertexBuffer, nullptr);
    vkFreeMemory(lveDevice.device(), vertexBufferMemory, nullptr);
  }

  void LveModel::bind(VkCommandBuffer commandBuffer) {
    // Associa il vertex buffer al command buffer a partire dall'indice di binding 0 con offset 0
    VkBuffer buffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
  }

  void LveModel::draw(VkCommandBuffer commandBuffer) {
    // Disegna tutti i vertici del modello (1 singola istanza, nessun offset per i vertici o istanze)
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
  }

  void LveModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    // Verifica che ci siano almeno 3 vertici per poter formare almeno un triangolo
    assert(vertexCount >= 3 && "Vertex count must be at least 3");
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

    // Crea il buffer con flag di utilizzo come VERTEX_BUFFER.
    // Proprietà di memoria:
    // - HOST_VISIBLE: accessibile direttamente dalla CPU per consentire la scrittura dei dati.
    // - HOST_COHERENT: assicura che le scritture della CPU siano automaticamente propagate/sincronizzate
    //   con la GPU senza dover chiamare esplicitamente vkFlushMappedMemoryRanges.
    lveDevice.createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vertexBuffer,
      vertexBufferMemory);

    // Mappa la memoria della GPU in uno spazio di indirizzamento accessibile dalla CPU (host pointer)
    void* data;
    vkMapMemory(lveDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
    // Copia i vertici dalla memoria host all'area mappata
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    // Rilascia la mappatura della memoria
    vkUnmapMemory(lveDevice.device(), vertexBufferMemory);
  }

  std::vector<VkVertexInputBindingDescription> LveModel::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    // Indice del binding corrispondente al nostro vertex buffer
    bindingDescriptions[0].binding = 0;
    // Stride: numero di byte da avanzare per passare al vertice successivo (dimensione di Vertex)
    bindingDescriptions[0].stride = sizeof(Vertex);
    // Avanza di uno stride per ciascun vertice (in alternativa a RATE_INSTANCE per il rendering istanziato)
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
  }

  std::vector<VkVertexInputAttributeDescription> LveModel::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
    // Binding a cui appartiene questo attributo
    attributeDescriptions[0].binding = 0;
    // Corrisponde a layout(location = 0) specificato nel vertex shader
    attributeDescriptions[0].location = 0;
    // Formato del dato: 3 float a 32-bit (vec3) per la posizione 3D
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    // Offset in byte dall'inizio della struct del vertice (0 poiché position è il primo attributo)
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    // 2° Attributo: Colore (interleaved nello stesso binding)
    attributeDescriptions[1].binding = 0;
    // Corrisponde a layout(location = 1) specificato nel vertex shader
    attributeDescriptions[1].location = 1;
    // Formato del dato: 3 float a 32-bit (vec3 RGB)
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    // Offset calcolato automaticamente con offsetof per individuare la posizione del membro 'color' nella struct
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
  }
}
// namespace lve