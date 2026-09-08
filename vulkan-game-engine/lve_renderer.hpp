#pragma once

#include "lve_device.hpp"
#include "lve_swap_chain.hpp"
#include "lve_window.hpp"
#include "vulkan/vulkan_core.h"

// std
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

namespace lve {
  // Renderer: responsabile della gestione del ciclo di vita della swap chain,
  // dell'allocazione dei command buffer e del controllo degli stadi del frame (begin/end frame e render pass).
  class LveRenderer {
  public:
    LveRenderer(LveWindow& lveWindow, LveDevice& lveDevice);
    ~LveRenderer();

    // Gestione del ciclo di vita: LveRenderer gestisce direttamente risorse Vulkan esplicite (command buffers),
    // pertanto eliminiamo costruttore di copia e operatore di assegnazione.
    LveRenderer(const LveRenderer&) = delete;
    LveRenderer& operator=(const LveRenderer&) = delete;

    // Restituisce true se un frame è attualmente in fase di registrazione
    bool isFrameInProgress() const { return isFrameStarted; };
    // Fornisce l'accesso al render pass della swap chain (necessario per configurare le pipeline nei vari render system)
    VkRenderPass getSwapChainRenderPass() const { return lveSwapChain->getRenderPass(); };
    // Restituisce il command buffer per il frame corrente (assicurandosi che il frame sia iniziato)
    VkCommandBuffer getCurrentCommandBuffer() const {
      assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
      return commandBuffers[currentFrameIndex];
    }

    // Restituisce l'indice del frame in volo (compreso tra 0 e MAX_FRAMES_IN_FLIGHT - 1)
    int getFrameIndex() const {
      assert(isFrameStarted && "Cannot get frame index when frame not in progress");
      return currentFrameIndex;
    }

    // Avvia un nuovo frame: acquisisce l'immagine dalla swap chain e avvia la registrazione del command buffer.
    // Restituisce nullptr se la swap chain è stata ricreata o non è pronta.
    VkCommandBuffer beginFrame();
    // Conclude il frame: termina la registrazione del command buffer, lo invia alla coda grafica e presenta l'immagine
    void endFrame();
    // Avvia il render pass della swap chain configurando framebuffer, render area, clear values, viewport e scissor dinamici
    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    // Conclude il render pass della swap chain
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

  private:
    void createCommandBuffers();
    // Dealloca i command buffer dal command pool
    void freeCommandBuffers();
    // Ricrea la swap chain quando la finestra viene ridimensionata
    void recreateSwapChain();

    LveWindow& lveWindow;
    LveDevice& lveDevice;
    // Swap chain: gestita tramite unique_ptr per consentire la distruzione e ricreazione dinamica al resize della finestra
    std::unique_ptr<LveSwapChain> lveSwapChain;
    // Command Buffer: registrano i comandi di rendering da inviare alla GPU (uno per ciascun frame in flight)
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t currentImageIndex;  // Indice dell'immagine corrente della swap chain acquisita
    int currentFrameIndex = 0;   // Indice del frame in volo corrente (0 .. MAX_FRAMES_IN_FLIGHT - 1)
    bool isFrameStarted = false; // Flag per tracciare se un frame è attualmente in corso
  };
}