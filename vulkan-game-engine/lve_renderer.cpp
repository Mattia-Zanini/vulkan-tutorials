#include "lve_renderer.hpp"
#include "lve_swap_chain.hpp"

// std
#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace lve {

  LveRenderer::LveRenderer(LveWindow& window, LveDevice& device)
    : lveWindow{ window }, lveDevice{ device } {
    // Inizializza la swap chain e alloca i command buffer per il rendering
    recreateSwapChain();
    createCommandBuffers();
  }

  LveRenderer::~LveRenderer() { freeCommandBuffers(); }

  void LveRenderer::recreateSwapChain() {
    auto extent = lveWindow.getExtent();
    // Gestione della minimizzazione della finestra: finché una delle dimensioni è 0,
    // mettiamo in pausa il programma e attendiamo nuovi eventi GLFW
    while (extent.width == 0 || extent.height == 0) {
      extent = lveWindow.getExtent();
      glfwWaitEvents();
    }

    // Attende che la GPU abbia terminato l'esecuzione dei comandi prima di distruggere o ricreare la swap chain
    vkDeviceWaitIdle(lveDevice.device());

    if (lveSwapChain == nullptr)
      lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extent);
    else {
      std::shared_ptr<LveSwapChain> oldSwapChain = std::move(lveSwapChain);
      // Ricrea la swap chain passando la precedente (tramite std::move) per agevolare il riuso delle risorse
      lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extent, oldSwapChain);

      // Se i formati di immagine o profondità cambiano, il render pass esistente non è più compatibile
      if (!oldSwapChain->compareSwapFormats(*lveSwapChain.get()))
        throw std::runtime_error("Swap chain image(or depth) format has chaged!");
    }
  }

  void LveRenderer::createCommandBuffers() {
    // Alloca un command buffer per ciascun frame contemporaneamente in elaborazione sulla GPU (MAX_FRAMES_IN_FLIGHT).
    // Disaccoppiando il numero di command buffer dal numero di immagini della swap chain,
    // non è necessario ricrearli ad ogni ricreazione della swap chain.
    commandBuffers.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // PRIMARY: il command buffer può essere inviato a una coda per l'esecuzione, ma non può essere
    // chiamato da altri buffer. (I SECONDARY non possono essere inviati direttamente alla coda, ma
    // possono essere chiamati da command buffer primari).
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // Command Pool: alloca e gestisce la memoria per i command buffer, ammortizzando i costi di
    // allocazione.
    allocInfo.commandPool = lveDevice.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(lveDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffers!");
    }
  }

  void LveRenderer::freeCommandBuffers() {
    // Rilascia la memoria di tutti i command buffer precedentemente allocati nel command pool
    vkFreeCommandBuffers(
      lveDevice.device(),
      lveDevice.getCommandPool(),
      static_cast<uint32_t>(commandBuffers.size()),
      commandBuffers.data());

    commandBuffers.clear();
  }

  VkCommandBuffer LveRenderer::beginFrame() {
    assert(isFrameStarted == false && "Can't call beginFrame while already in progress");

    // Ottiene l'indice della prossima immagine disponibile nella swap chain su cui renderizzare.
    // Gestisce automaticamente la sincronizzazione CPU/GPU (fences e semafori) per double/triple buffering.
    auto result = lveSwapChain->acquireNextImage(&currentImageIndex);

    // Se la swap chain non è più valida (es. resize immediato della finestra), la ricrea ed esce dal frame restituendo nullptr
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapChain();
      return nullptr; // Il frame non è iniziato con successo; il chiamante non eseguirà la registrazione dei comandi
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    isFrameStarted = true;
    auto commandBuffer = getCurrentCommandBuffer();

    // Avvia la registrazione del command buffer per il frame corrente
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
    }

    return commandBuffer;
  }

  void LveRenderer::endFrame() {
    assert(isFrameStarted && "Can't call endFrame while frame is not in progress");
    auto commandBuffer = getCurrentCommandBuffer();

    // Conclude la registrazione dei comandi nel command buffer
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }

    // Invia il command buffer corrispondente alla graphics queue del device ed esegue il comando.
    // La swap chain presenterà poi a schermo l'immagine renderizzata al momento opportuno (in base al present mode).
    auto result = lveSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

    // Se la superficie è cambiata, è subottimale oppure è stato intercettato l'evento di resize dalla callback GLFW,
    // resettiamo il flag e ricreiamo la swap chain
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || lveWindow.wasWindowResized()) {
      lveWindow.resetWindowResizedFlag();
      recreateSwapChain();
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
    }

    isFrameStarted = false;
    // Avanza ciclicamente l'indice del frame in volo [0, MAX_FRAMES_IN_FLIGHT - 1]
    currentFrameIndex = (currentFrameIndex + 1) % LveSwapChain::MAX_FRAMES_IN_FLIGHT;
  }

  void LveRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "Can't call beginSwapChainRenderPass while frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() && "Can't beging render pass on command buffer from a different frame");

    // Configurazione dell'inizio del Render Pass per il framebuffer associato all'immagine corrente della swap chain
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = lveSwapChain->getRenderPass();
    renderPassInfo.framebuffer = lveSwapChain->getFrameBuffer(currentImageIndex);

    // Render area: definisce la regione in cui avvengono i load/store degli shader.
    // Si utilizza l'extent della swap chain (pixel reali) e non quello della finestra.
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = lveSwapChain->getSwapChainExtent();

    // Valori di clear iniziali per gli attachment definiti nel Render Pass:
    // Indice 0: Color attachment (colore di sfondo RGB + Alpha)
    // Indice 1: Depth/Stencil attachment (valore di profondità iniziale = 1.0, punto più lontano)
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // Inizio del render pass. VK_SUBPASS_CONTENTS_INLINE indica che i comandi del render pass
    // sono incorporati direttamente in questo primary command buffer (senza uso di secondary
    // command buffer).
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Configurazione dinamica di Viewport e Scissor:
    // Poiché sono stati dichiarati come stati dinamici (VK_DYNAMIC_STATE_VIEWPORT/SCISSOR),
    // possiamo aggiornarne le dimensioni in tempo reale nel command buffer per adattarli alla swap chain
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(lveSwapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(lveSwapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{ { 0, 0 }, lveSwapChain->getSwapChainExtent() };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
  }

  void LveRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "Can't call endSwapChainRenderPass while frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame");

    // Termina il render pass e conclude la registrazione del command buffer
    vkCmdEndRenderPass(commandBuffer);
  }

} // namespace lve