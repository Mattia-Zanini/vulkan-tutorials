#include "first_app.hpp"
#include "lve_model.hpp"
#include "vulkan/vulkan_core.h"

// std
#include <array>
#include <memory>
#include <stdexcept>
#include <vector>

namespace lve {

  FirstApp::FirstApp() {
    // Inizializza le risorse Vulkan necessarie: layout della pipeline, la pipeline grafica e i
    // command buffers
    loadModels();
    createPipelineLayout();
    createPipeline();
    createCommandBuffers();
  }

  FirstApp::~FirstApp() {
    // Distruzione esplicita del layout della pipeline al termine dell'applicazione
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
  }

  void FirstApp::run() {
    while (!lveWindow.shouldClose()) {
      glfwPollEvents();
      drawFrame();
    }

    // Attende che la GPU abbia completato tutte le operazioni prima di uscire dall'applicazione.
    // Evita errori e messaggi dai validation layers dovuti alla distruzione di risorse ancora in
    // uso dalla GPU.
    vkDeviceWaitIdle(lveDevice.device());
  }

  void generateSierpinski(
    std::vector<LveModel::Vertex>& vertices,
    int depth,
    glm::vec2 a,
    glm::vec2 b,
    glm::vec2 c) {
    // caso base
    if (depth == 0) {
      vertices.push_back(LveModel::Vertex{ a });
      vertices.push_back(LveModel::Vertex{ b });
      vertices.push_back(LveModel::Vertex{ c });
      return;
    }

    // trovo i punti medi
    glm::vec2 ab = (a + b) / 2.0f;
    glm::vec2 bc = (b + c) / 2.0f;
    glm::vec2 ca = (c + a) / 2.0f;

    // chiamate ricorsive
    generateSierpinski(vertices, depth - 1, a, ab, ca); // triangolo in alto
    generateSierpinski(vertices, depth - 1, ab, b, bc); // triangolo a destra
    generateSierpinski(vertices, depth - 1, ca, bc, c); // triangolo a sinistra
  }

  void FirstApp::loadModels() {
    // Definiamo le coordinate 2D dei vertici del triangolo (x, y) nello spazio normalizzato [-1, 1]
    glm::vec2 a = { 0.0f, -0.8f };
    glm::vec2 b = { 0.8f, 0.8f };
    glm::vec2 c = { -0.8f, 0.8f };

    // Definiamo i colori primari (rosso, verde, blu) per ciascun vertice del triangolo
    glm::vec3 red = { 1.0f, 0.0f, 0.0f };
    glm::vec3 green = { 0.0f, 1.0f, 0.0f };
    glm::vec3 blue = { 0.0f, 0.0f, 1.0f };

    // Inizializziamo i vertici combinando posizione 2D e colore RGB (interleaved).
    // Lo stadio di rasterizzazione calcolerà automaticamente le coordinate baricentriche per interpolare
    // sfumature morbide tra i vertici su ciascun pixel/frammento del triangolo.
    std::vector<LveModel::Vertex> vertices{
      { a, red },
      { b, green },
      { c, blue }
    };

    // Alloca il vertex buffer sulla GPU e copia i dati dei vertici tramite LveModel
    lveModel = std::make_unique<LveModel>(lveDevice, vertices);
  }

  void FirstApp::createPipelineLayout() {
    // Pipeline Layout: definisce come passare dati agli shader oltre ai dati dei vertici.
    // Include descrittori (texture, Uniform Buffer Objects) e push constants.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // Per ora creiamo un layout vuoto (senza set layouts e senza push constants)
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    // Push constants: metodo estremamente efficiente per inviare piccole quantità di dati agli
    // shader
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
    }
  }

  void FirstApp::createPipeline() {
    // Utilizziamo le dimensioni (width/height) della swap chain e non della finestra:
    // su display ad alta densità (es. Retina su macOS), le coordinate finestra differiscono dal
    // numero reale di pixel.
    auto pipelineConfig = LvePipeline::defaultPipelineConfigInfo(lveSwapChain.width(), lveSwapChain.height());

    // Il render pass funge da "blueprint" che descrive la struttura del framebuffer (attachment di
    // colore, depth, ecc.) La pipeline deve sapere in anticipo con quale layout di render pass sarà
    // compatibile per produrre l'output corretto.
    pipelineConfig.renderPass = lveSwapChain.getRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;

    // Creiamo l'istanza della pipeline mediante unique_ptr
    lvePipeline = std::make_unique<LvePipeline>(lveDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);
  }

  void FirstApp::createCommandBuffers() {
    // In Vulkan i comandi di disegno non vengono eseguiti direttamente tramite chiamate a funzioni,
    // ma vengono registrati in un command buffer e poi sottomessi alla coda del device.
    // Creiamo un command buffer per ciascuna immagine della swap chain (relazione 1:1 con i
    // framebuffer), registrandoli una sola volta all'avvio per poi riutilizzarli a ogni frame.
    commandBuffers.resize(lveSwapChain.imageCount());

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

    // Registrazione dei comandi di disegno per ciascun command buffer
    for (int i = 0; i < commandBuffers.size(); i++) {
      VkCommandBufferBeginInfo beginInfo{};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

      if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
      }

      // Configurazione dell'inizio del Render Pass
      VkRenderPassBeginInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = lveSwapChain.getRenderPass();
      renderPassInfo.framebuffer = lveSwapChain.getFrameBuffer(i);

      // Render area: definisce la regione in cui avvengono i load/store degli shader.
      // Si utilizza l'extent della swap chain (pixel reali) e non quello della finestra.
      renderPassInfo.renderArea.offset = { 0, 0 };
      renderPassInfo.renderArea.extent = lveSwapChain.getSwapChainExtent();

      // Valori di clear iniziali per gli attachment definiti nel Render Pass:
      // Indice 0: Color attachment (colore di sfondo RGB + Alpha)
      // Indice 1: Depth/Stencil attachment (valore di profondità iniziale = 1.0, punto più lontano)
      std::array<VkClearValue, 2> clearValues{};
      clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
      clearValues[1].depthStencil = { 1.0f, 0 };
      renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
      renderPassInfo.pClearValues = clearValues.data();

      // Inizio del render pass. VK_SUBPASS_CONTENTS_INLINE indica che i comandi del render pass
      // sono incorporati direttamente in questo primary command buffer (senza uso di secondary
      // command buffer).
      vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

      // Associa la pipeline grafica, poi associa il vertex buffer del modello ed esegue il draw
      lvePipeline->bind(commandBuffers[i]);
      lveModel->bind(commandBuffers[i]);
      lveModel->draw(commandBuffers[i]);

      // Termina il render pass e conclude la registrazione del command buffer
      vkCmdEndRenderPass(commandBuffers[i]);
      if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
      }
    }
  }

  void FirstApp::drawFrame() {
    uint32_t imageIndex;
    // Ottiene l'indice della prossima immagine disponibile nella swap chain su cui renderizzare.
    // Gestisce automaticamente la sincronizzazione CPU/GPU (fences e semafori) per double/triple
    // buffering.
    auto result = lveSwapChain.acquireNextImage(&imageIndex);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Invia il command buffer corrispondente alla graphics queue del device ed esegue il comando.
    // La swap chain presenterà poi a schermo l'immagine renderizzata al momento opportuno (in base
    // al present mode).
    result = lveSwapChain.submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
    if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
    }
  }
} // namespace lve