#pragma once

#include "lve_device.hpp"

// libs
#include <vulkan/vulkan.h>

// std
#include <memory>
#include <string>

namespace lve {
  // Incapsula una texture 2D in Vulkan: gestisce il caricamento dell'immagine,
  // l'allocazione della memoria GPU (VkImage), la vista (VkImageView) e il campionatore (VkSampler)
  class LveTexture {
  public:
    // Costruttore che carica un'immagine da file (PNG/JPEG) tramite stb_image
    LveTexture(LveDevice& device, const std::string& textureFilepath);
    LveTexture(
      LveDevice& device,
      VkFormat format,
      VkExtent3D extent,
      VkImageUsageFlags usage,
      VkSampleCountFlagBits sampleCount);
    ~LveTexture();

    // Impedisce la copia accidentale della texture (risorse GPU univoche)
    LveTexture(const LveTexture&) = delete;
    LveTexture& operator=(const LveTexture&) = delete;

    VkImageView imageView() const { return mTextureImageView; }
    VkSampler sampler() const { return mTextureSampler; }
    VkImage getImage() const { return mTextureImage; }
    VkImageView getImageView() const { return mTextureImageView; }
    // Restituisce la struttura VkDescriptorImageInfo pronta per l'aggiornamento dei descriptor set
    VkDescriptorImageInfo getImageInfo() const { return mDescriptor; }
    VkImageLayout getImageLayout() const { return mTextureLayout; }
    VkExtent3D getExtent() const { return mExtent; }
    VkFormat getFormat() const { return mFormat; }

    // Aggiorna il descrittore interno combinando sampler, image view e layout
    void updateDescriptor();
    // Registra una pipeline barrier per la transizione del layout sul command buffer specificato
    void transitionLayout(
      VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);

    // Factory method per creare un'istanza di LveTexture leggendo il file da disco
    static std::unique_ptr<LveTexture> createTextureFromFile(
      LveDevice& device, const std::string& filepath);

  private:
    void createTextureImage(const std::string& filepath);
    void createTextureImageView(VkImageViewType viewType);
    void createTextureSampler();

    VkDescriptorImageInfo mDescriptor{};

    LveDevice& mDevice;
    VkImage mTextureImage = nullptr;
    VkDeviceMemory mTextureImageMemory = nullptr;
    VkImageView mTextureImageView = nullptr;
    VkSampler mTextureSampler = nullptr;
    VkFormat mFormat;
    VkImageLayout mTextureLayout;
    uint32_t mMipLevels{ 1 };
    uint32_t mLayerCount{ 1 };
    VkExtent3D mExtent{};
  };

} // namespace lve