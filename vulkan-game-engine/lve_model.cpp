#include "lve_model.hpp"
#include "vulkan/vulkan_core.h"

// std
#include <cassert>
#include <cstdint>
#include <cstring>

namespace lve {
  LveModel::LveModel(LveDevice& device, const LveModel::Builder& builder) : lveDevice{ device } {
    createVertexBuffers(builder.vertices);
    createIndexBuffers(builder.indices);
  }

  LveModel::~LveModel() {
    // Rilascio esplicito del buffer e della porzione di memoria allocata sulla GPU
    vkDestroyBuffer(lveDevice.device(), vertexBuffer, nullptr);
    vkFreeMemory(lveDevice.device(), vertexBufferMemory, nullptr);

    if (hasIndexBuffer) {
      vkDestroyBuffer(lveDevice.device(), indexBuffer, nullptr);
      vkFreeMemory(lveDevice.device(), indexBufferMemory, nullptr);
    }
  }

  void LveModel::bind(VkCommandBuffer commandBuffer) {
    // Esegue il bind dei vertici necessari per il disegno
    VkBuffer buffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    // Se il modello contiene un index buffer, ne esegue il bind specificando
    // il tipo di dato degli indici (VK_INDEX_TYPE_UINT32, per supportare modelli complessi)
    if (hasIndexBuffer)
      vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  }

  void LveModel::draw(VkCommandBuffer commandBuffer) {
    if (hasIndexBuffer)
      // Disegna il modello sfruttando l'index buffer (riutilizzo dei vertici)
      vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
    else
      // Disegna tutti i vertici del modello in modo sequenziale se non c'è index buffer
      vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
  }

  void LveModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount >= 3 && "Vertex count must be at least 3");
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

    // Staging Buffer: buffer temporaneo visibile dalla CPU dove carichiamo inizialmente i dati.
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // Crea lo staging buffer. TRANSFER_SRC_BIT indica che sarà la sorgente di un'operazione di copia memoria.
    // Proprietà di memoria:
    // - HOST_VISIBLE: accessibile direttamente dalla CPU per consentire la scrittura dei dati.
    // - HOST_COHERENT: assicura che le scritture della CPU siano automaticamente propagate/sincronizzate
    //   con la GPU senza dover chiamare esplicitamente vkFlushMappedMemoryRanges.
    lveDevice.createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      stagingBuffer,
      stagingBufferMemory);

    // Mappa la memoria della GPU in uno spazio accessibile dalla CPU, ci copia i vertici e la rilascia
    void* data;
    vkMapMemory(lveDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(lveDevice.device(), stagingBufferMemory);

    // Crea il vertex buffer vero e proprio. TRANSFER_DST_BIT indica che è la destinazione della copia.
    // DEVICE_LOCAL_BIT indica la memoria più veloce della GPU, che però non è accessibile dalla CPU.
    // L'utilizzo di uno staging buffer e DEVICE_LOCAL è ideale per dati statici (come le mesh 3D),
    // mentre aggiornamenti frequenti (es. per ogni frame) annullerebbero i benefici a causa del costo della copia.
    lveDevice.createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      vertexBuffer,
      vertexBufferMemory);

    // Copia i dati dallo staging buffer al buffer finale ad alte prestazioni
    lveDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // Lo staging buffer non serve più, possiamo eliminarlo
    vkDestroyBuffer(lveDevice.device(), stagingBuffer, nullptr);
    vkFreeMemory(lveDevice.device(), stagingBufferMemory, nullptr);
  }

  void LveModel::createIndexBuffers(const std::vector<uint32_t>& indices) {
    indexCount = static_cast<uint32_t>(indices.size());
    hasIndexBuffer = indexCount > 0;

    // Se non vengono forniti indici, l'index buffer non viene creato
    if (hasIndexBuffer == false)
      return;

    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

    // Anche per l'index buffer usiamo la tecnica dello staging buffer per
    // trasferire i dati sulla memoria DEVICE_LOCAL (ottimale per la GPU)
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    lveDevice.createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      stagingBuffer,
      stagingBufferMemory);

    void* data;
    vkMapMemory(lveDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    // Copia gli indici invece dei vertici
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(lveDevice.device(), stagingBufferMemory);

    // Crea l'index buffer vero e proprio con i flag appropriati (INDEX_BUFFER_BIT)
    lveDevice.createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      indexBuffer,
      indexBufferMemory);

    lveDevice.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(lveDevice.device(), stagingBuffer, nullptr);
    vkFreeMemory(lveDevice.device(), stagingBufferMemory, nullptr);
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