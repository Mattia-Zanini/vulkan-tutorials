#pragma once

#include "lve_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace lve {

  // Incapsula un VkDescriptorSetLayout definendo il "blueprint" (struttura) dei descrittori
  // attesi dalla pipeline: quali tipi di risorse aspettarsi, a quali indici di binding e per quali stadi dello shader.
  class LveDescriptorSetLayout {
  public:
    // Builder per facilitare la configurazione e il concatenamento (chaining) dei binding prima della creazione del layout.
    class Builder {
    public:
      Builder(LveDevice& lveDevice) : lveDevice{ lveDevice } {}

      Builder& addBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        uint32_t count = 1);
      std::unique_ptr<LveDescriptorSetLayout> build() const;

    private:
      LveDevice& lveDevice;
      std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
    };

    LveDescriptorSetLayout(LveDevice& lveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
    ~LveDescriptorSetLayout();

    LveDescriptorSetLayout(const LveDescriptorSetLayout&) = delete;
    LveDescriptorSetLayout& operator=(const LveDescriptorSetLayout&) = delete;

    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

  private:
    LveDevice& lveDevice;
    VkDescriptorSetLayout descriptorSetLayout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

    friend class LveDescriptorWriter;
  };

  // Incapsula un VkDescriptorPool per allocare la memoria dei descriptor set in blocchi,
  // evitando allocazioni frequenti e costose sulla GPU.
  class LveDescriptorPool {
  public:
    // Builder per configurare il numero massimo di set allocabili e la quantità per ciascun tipo di descrittore.
    class Builder {
    public:
      Builder(LveDevice& lveDevice) : lveDevice{ lveDevice } {}

      Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
      Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
      Builder& setMaxSets(uint32_t count);
      std::unique_ptr<LveDescriptorPool> build() const;

    private:
      LveDevice& lveDevice;
      std::vector<VkDescriptorPoolSize> poolSizes{};
      uint32_t maxSets = 1000;
      VkDescriptorPoolCreateFlags poolFlags = 0;
    };

    LveDescriptorPool(
      LveDevice& lveDevice,
      uint32_t maxSets,
      VkDescriptorPoolCreateFlags poolFlags,
      const std::vector<VkDescriptorPoolSize>& poolSizes);
    ~LveDescriptorPool();

    LveDescriptorPool(const LveDescriptorPool&) = delete;
    LveDescriptorPool& operator=(const LveDescriptorPool&) = delete;

    // Alloca un intero descriptor set dal pool in base al layout specificato
    bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;
    void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;
    void resetPool();

  private:
    LveDevice& lveDevice;
    VkDescriptorPool descriptorPool;

    friend class LveDescriptorWriter;
  };

  // Classe di supporto (friend di Layout e Pool) che gestisce l'allocazione dal pool
  // e la scrittura delle risorse nei descriptor set tramite comandi vkUpdateDescriptorSets.
  class LveDescriptorWriter {
  public:
    LveDescriptorWriter(LveDescriptorSetLayout& setLayout, LveDescriptorPool& pool);

    // Registra la scrittura di un buffer al binding specificato
    LveDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
    // Registra la scrittura di un'immagine al binding specificato
    LveDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

    // Alloca il descriptor set dal pool ed esegue la scrittura delle risorse registrate
    bool build(VkDescriptorSet& set);
    // Esegue solo l'aggiornamento (overwrite) su un descriptor set già allocato
    void overwrite(VkDescriptorSet& set);

  private:
    LveDescriptorSetLayout& setLayout;
    LveDescriptorPool& pool;
    std::vector<VkWriteDescriptorSet> writes;
  };

} // namespace lve