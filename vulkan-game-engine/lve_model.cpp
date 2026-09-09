#include "lve_model.hpp"
#include "lve_buffer.hpp"
#include "lve_utils.hpp"
#include "vulkan/vulkan_core.h"

// libs
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// std
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace std {
  // Specializzazione del template std::hash per la struttura Vertex.
  // Serve all'unordered_map per generare un hash unico (size_t) partendo da tutti i dati del vertice,
  // permettendoci di individuare velocemente vertici duplicati.
  //
  // Di norma, aggiungere cose a namespace std è vietato in C++, tranne per un'unica eccezione concessa
  // dallo standard: puoi aprire namespace std per fornire la specializzazione di un template per un
  // tuo tipo personalizzato.
  //
  // In parole semplici è come se stessi dicendo al compilatore: "Quando qualcuno ti chiede di calcolare
  // std::hash sul tipo specifico lve::LveModel::Vertex, non usare il template generico: usa questa
  // definizione esatta che ti sto scrivendo qui."
  template <>
  struct hash<lve::LveModel::Vertex> {
    size_t operator()(lve::LveModel::Vertex const& vertex) const {
      // Questo definisce un Functor (oggetto funzione).
      // Permette a una classe o struct di essere invocata come se fosse una funzione normale usando
      // le parentesi tonde ().
      // Quando std::unordered_map deve calcolare l'hash di un vertice v, chiama internamente:
      // size_t hashValue = std::hash<Vertex>{}(v); // Crea l'oggetto hash e invoca operator()(v)

      size_t seed = 0;
      lve::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
      return seed;
    }
  };
} // namespace std

namespace lve {
  LveModel::LveModel(LveDevice& device, const LveModel::Builder& builder) : lveDevice{ device } {
    createVertexBuffers(builder.vertices);
    createIndexBuffers(builder.indices);
  }

  // La distruzione del buffer e il rilascio della memoria sono ora gestiti automaticamente da LveBuffer
  LveModel::~LveModel() {}

  std::unique_ptr<LveModel> LveModel::createModelFromFile(LveDevice& device, const std::string& filepath) {
    Builder builder{};
    builder.loadModel(filepath);
    spdlog::info("Vertex count: {}", builder.vertices.size());

    return std::make_unique<LveModel>(device, builder);
  }

  void LveModel::bind(VkCommandBuffer commandBuffer) {
    // Esegue il bind dei vertici necessari per il disegno
    VkBuffer buffers[] = { vertexBuffer->getBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    // Se il modello contiene un index buffer, ne esegue il bind specificando
    // il tipo di dato degli indici (VK_INDEX_TYPE_UINT32, per supportare modelli complessi)
    if (hasIndexBuffer)
      vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
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
    uint32_t vertexSize = sizeof(vertices[0]);

    // Utilizza la nuova classe di astrazione LveBuffer per creare lo staging buffer (host visible/coherent)
    LveBuffer stagingBuffer{
      lveDevice,
      vertexSize,
      vertexCount,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    // Mappa la memoria e scrive i dati dei vertici tramite i metodi di supporto di LveBuffer
    stagingBuffer.map();
    stagingBuffer.writeToBuffer((void*)vertices.data());

    // Crea il vertex buffer finale su memoria DEVICE_LOCAL (ottimale per la GPU)
    vertexBuffer = std::make_unique<LveBuffer>(
      lveDevice,
      vertexSize,
      vertexCount,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Copia i dati dallo staging buffer al buffer finale ad alte prestazioni
    lveDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
    // Lo staging buffer (allocato sullo stack) viene deallocato e ripulito automaticamente al termine dello scope
  }

  void LveModel::createIndexBuffers(const std::vector<uint32_t>& indices) {
    indexCount = static_cast<uint32_t>(indices.size());
    hasIndexBuffer = indexCount > 0;

    // Se non vengono forniti indici, l'index buffer non viene creato
    if (hasIndexBuffer == false)
      return;

    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
    uint32_t indexSize = sizeof(indices[0]);

    // Crea lo staging buffer per gli indici tramite LveBuffer
    LveBuffer stagingBuffer{
      lveDevice,
      indexSize,
      indexCount,
      VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    stagingBuffer.map();
    stagingBuffer.writeToBuffer((void*)indices.data());

    // Crea l'index buffer vero e proprio su memoria DEVICE_LOCAL
    indexBuffer = std::make_unique<LveBuffer>(
      lveDevice,
      indexSize,
      indexCount,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Copia i dati dallo staging buffer al buffer finale ad alte prestazioni
    lveDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
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
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

    // Configura i 4 attributi per i vertici: posizione, colore, normale (per illuminazione diffusa) e coordinate UV
    attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
    attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
    attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
    attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });

    return attributeDescriptions;
  }

  void LveModel::Builder::loadModel(const std::string& filepath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Utilizza tinyobjloader per parsare il file .obj. Popolerà attrib (posizioni, colori, normali, uvs)
    // e shapes (lista dei volti/facce, ciascuno contenente le triplette di indici per i propri vertici)
    if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()) == false)
      throw std::runtime_error(warn + err);

    vertices.clear();
    indices.clear();

    // unordered_map per mappare vertici univoci all'indice corrispondente all'interno del vettore 'vertices'
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    for (const auto& shape : shapes) {
      for (const auto& index : shape.mesh.indices) {
        Vertex vertex{};

        // Estrae la posizione dal vettore piatto (con valori raggruppati a 3 a 3)
        if (index.vertex_index >= 0) {
          vertex.position = {
            attrib.vertices[3 * index.vertex_index + 0],
            attrib.vertices[3 * index.vertex_index + 1],
            attrib.vertices[3 * index.vertex_index + 2],
          };

          // In tinyobjloader attrib.colors ha la stessa dimensione di attrib.vertices ed è precompilato
          // con valori 1.0 (bianco) di default quando il colore non è specificato nel file .obj
          vertex.color = {
            attrib.colors[3 * index.vertex_index + 0],
            attrib.colors[3 * index.vertex_index + 1],
            attrib.colors[3 * index.vertex_index + 2],
          };
        }

        // Estrae le normali (3 componenti)
        if (index.normal_index >= 0) {
          vertex.normal = {
            attrib.normals[3 * index.normal_index + 0],
            attrib.normals[3 * index.normal_index + 1],
            attrib.normals[3 * index.normal_index + 2],
          };
        }

        // Estrae le coordinate UV (2 componenti)
        if (index.texcoord_index >= 0) {
          vertex.uv = {
            attrib.texcoords[2 * index.texcoord_index + 0],
            attrib.texcoords[2 * index.texcoord_index + 1],
          };
        }

        // Se il vertice non è mai stato incontrato, lo inseriamo nel vettore dei vertici univoci
        if (uniqueVertices.count(vertex) == 0) {
          uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
          vertices.push_back(vertex);
        }
        // In ogni caso, aggiungiamo l'indice del vertice univoco al nostro index buffer
        indices.push_back(uniqueVertices[vertex]);
      }
    }
  }
}
// namespace lve