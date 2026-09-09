#pragma once

#include "lve_device.hpp"

namespace lve {

  // Incapsula un VkBuffer e la relativa VkDeviceMemory allocata sulla GPU in un unico oggetto RAII.
  // Fornisce metodi per le operazioni comuni (mappatura memoria, scrittura dall'host, flush selettivo)
  // e supporta l'allineamento automatico (minOffsetAlignment) per raggruppare più istanze (es. UBO per frame).
  class LveBuffer {
  public:
    // Costruttore: calcola l'allineamento richiesto, determina la dimensione totale (alignmentSize * instanceCount)
    // e alloca il buffer e la memoria del dispositivo.
    LveBuffer(
      LveDevice& device,
      VkDeviceSize instanceSize,
      uint32_t instanceCount,
      VkBufferUsageFlags usageFlags,
      VkMemoryPropertyFlags memoryPropertyFlags,
      VkDeviceSize minOffsetAlignment = 1);
    ~LveBuffer();

    LveBuffer(const LveBuffer&) = delete;
    LveBuffer& operator=(const LveBuffer&) = delete;

    // Mappa un intervallo di memoria del buffer rendendolo accessibile come puntatore dalla CPU (host)
    VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    // Rilascia la mappatura della memoria del buffer
    void unmap();

    // Copia dati dalla memoria host all'area del buffer mappata
    void writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    // Esegue il flush esplicito per propagare le modifiche dalla CPU alla GPU (necessario se la memoria non è HOST_COHERENT)
    VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    // Restituisce le informazioni del descrittore del buffer per il binding nei descriptor set
    VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    // Invalida l'intervallo di memoria mappato per aggiornare le letture della CPU con le modifiche GPU
    VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    // Varianti indicizzate: permettono di operare direttamente sulla specifica istanza (es. per il frameIndex corrente)
    // tenendo conto dell'alignmentSize tra istanze consecutive nello stesso buffer.
    void writeToIndex(void* data, int index);
    VkResult flushIndex(int index);
    VkDescriptorBufferInfo descriptorInfoForIndex(int index);
    VkResult invalidateIndex(int index);

    // Getters per le proprietà del buffer
    VkBuffer getBuffer() const { return buffer; }
    void* getMappedMemory() const { return mapped; }
    uint32_t getInstanceCount() const { return instanceCount; }
    VkDeviceSize getInstanceSize() const { return instanceSize; }
    VkDeviceSize getAlignmentSize() const { return instanceSize; }
    VkBufferUsageFlags getUsageFlags() const { return usageFlags; }
    VkMemoryPropertyFlags getMemoryPropertyFlags() const { return memoryPropertyFlags; }
    VkDeviceSize getBufferSize() const { return bufferSize; }

  private:
    // Calcola l'offset o dimensione allineata minima richiesta per essere compatibile con i limiti del dispositivo
    static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

    LveDevice& lveDevice;
    void* mapped = nullptr;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;

    VkDeviceSize bufferSize;
    uint32_t instanceCount;
    VkDeviceSize instanceSize;
    VkDeviceSize alignmentSize;
    VkBufferUsageFlags usageFlags;
    VkMemoryPropertyFlags memoryPropertyFlags;
  };

} // namespace lve
