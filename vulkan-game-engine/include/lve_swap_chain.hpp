#pragma once

#include "lve_device.hpp"

// libs
#include <vulkan/vulkan.h>

// std
#include <memory>
#include <vector>

namespace lve {
  // Swap Chain: serie di frame buffer utilizzati per mostrare le immagini a video (window surface).
  // Coordina lo scambio (swap) tra:
  // - "Front Buffer": l'immagine attualmente visualizzata a schermo.
  // - "Back Buffer(s)": le immagini su cui la GPU sta renderizzando i frame successivi.
  // Gestisce la sincronizzazione (double buffering o triple buffering) in base alle capacità del
  // dispositivo, prevenendo il tearing (quando lo schermo mostra frammenti di frame diversi
  // simultaneamente) e creando i framebuffer e gli attachment (colore e profondità) per la pipeline
  // grafica.
  class LveSwapChain {
  public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    LveSwapChain(LveDevice& deviceRef, VkExtent2D windowExtent);
    // Costruttore con passaggio della vecchia swap chain (permette di riutilizzare risorse e fluidificare il resize)
    LveSwapChain(LveDevice& deviceRef, VkExtent2D windowExtent, std::shared_ptr<LveSwapChain> previous);
    ~LveSwapChain();

    LveSwapChain(const LveSwapChain&) = delete;
    void operator=(const LveSwapChain&) = delete;

    VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
    VkRenderPass getRenderPass() { return renderPass; }
    VkImageView getImageView(int index) { return swapChainImageViews[index]; }
    size_t imageCount() { return swapChainImages.size(); }
    VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return swapChainExtent; }
    uint32_t width() { return swapChainExtent.width; }
    uint32_t height() { return swapChainExtent.height; }

    float extentAspectRatio() { return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height); }
    VkFormat findDepthFormat();

    VkResult acquireNextImage(uint32_t* imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

    // Verifica la compatibilità dei formati (colore e profondità) tra la swap chain corrente e una nuova.
    // Se i formati coincidono, il render pass esistente rimane compatibile e le pipeline grafiche non necessitano di essere ricreate.
    bool compareSwapFormats(const LveSwapChain& swapChain) const {
      return swapChain.swapChainDepthFormat == swapChainDepthFormat && swapChain.swapChainImageFormat == swapChainImageFormat;
    }

  private:
    void init();
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();

    // Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VkFormat swapChainImageFormat;
    // Formato dell'attachment di profondità della swap chain (tracciato per verificare la compatibilità del render pass al resize)
    VkFormat swapChainDepthFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemorys;
    std::vector<VkImageView> depthImageViews;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    LveDevice& device;
    VkExtent2D windowExtent;

    VkSwapchainKHR swapChain;
    // Riferimento alla precedente swap chain utilizzato solo durante la fase di creazione/inizializzazione
    std::shared_ptr<LveSwapChain> oldSwapChain;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
  };

} // namespace lve
