#include "first_app.hpp"
#include "GLFW/glfw3.h"
#include "lve_model.hpp"
#include "lve_pipeline.hpp"
#include "lve_swap_chain.hpp"
#include "vulkan/vulkan_core.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace lve {

  // Struttura dati per le Push Constants: permette di passare piccoli blocchi di dati ai vari stadi dello shader
  // direttamente tramite il command buffer (senza allocazioni di memoria o descrittori).
  // Lo standard Vulkan garantisce almeno 128 byte condivisi tra tutti gli stadi.
  struct SimplePushConstantData {
    glm::vec2 offset;
    // In memoria GPU (regole di allineamento std430/std140), un vec3 deve essere allineato a un multiplo di 16 byte (4N).
    // Usiamo alignas(16) per forzare lo stesso padding di 8 byte anche nella struct host C++, evitando disallineamenti di lettura.
    alignas(16) glm::vec3 color;
  };

  FirstApp::FirstApp() {
    // Inizializza le risorse Vulkan necessarie: modelli, layout della pipeline, swap chain e command buffers
    loadModels();
    createPipelineLayout();
    // Crea la swap chain iniziale e la pipeline associata
    recreateSwapChain();
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
    LveModel::Vertex a,
    LveModel::Vertex b,
    LveModel::Vertex c) {
    // caso base
    if (depth == 0) {
      vertices.push_back(a);
      vertices.push_back(b);
      vertices.push_back(c);
      return;
    }

    // trovo i punti medi sia per la posizione (vec2) che per il colore (vec3)
    LveModel::Vertex ab{ (a.position + b.position) * 0.5f, (a.color + b.color) * 0.5f };
    LveModel::Vertex bc{ (b.position + c.position) * 0.5f, (b.color + c.color) * 0.5f };
    LveModel::Vertex ca{ (c.position + a.position) * 0.5f, (c.color + a.color) * 0.5f };

    // chiamate ricorsiv
    generateSierpinski(vertices, depth - 1, a, ab, ca); // triangolo in alto
    generateSierpinski(vertices, depth - 1, ab, b, bc); // triangolo a destra
    generateSierpinski(vertices, depth - 1, ca, bc, c); // triangolo a sinistra
  }

  void FirstApp::loadModels() {
    // Definiamo le coordinate 2D dei vertici del triangolo (x, y) nello spazio normalizzato [-1, 1]
    glm::vec2 a = { 0.0f, -0.5f };
    glm::vec2 b = { 0.5f, 0.5f };
    glm::vec2 c = { -0.5f, 0.5f };

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
    // Configura il range di push constants: specifica quali stadi dello shader possono accedervi,
    // l'offset di partenza (0) e la dimensione totale occupata in byte.
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    // Pipeline Layout: definisce come passare dati agli shader oltre ai dati dei vertici.
    // Include descrittori (texture, Uniform Buffer Objects) e push constants.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // Per ora creiamo un layout vuoto per i set di descrittori
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    // Collega il range di push constants al layout della pipeline grafica
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
    }
  }

  void FirstApp::createPipeline() {
    assert(lveSwapChain != nullptr && "Cannot create pipeline before swap chain");
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    // Utilizziamo le dimensioni (width/height) della swap chain e non della finestra:
    // su display ad alta densità (es. Retina su macOS), le coordinate finestra differiscono dal
    // numero reale di pixel.
    LvePipeline::defaultPipelineConfigInfo(pipelineConfig);

    // Il render pass funge da "blueprint" che descrive la struttura del framebuffer (attachment di
    // colore, depth, ecc.) La pipeline deve sapere in anticipo con quale layout di render pass sarà
    // compatibile per produrre l'output corretto.
    pipelineConfig.renderPass = lveSwapChain->getRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;

    // Creiamo l'istanza della pipeline mediante unique_ptr
    lvePipeline = std::make_unique<LvePipeline>(lveDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);
  }

  void FirstApp::recreateSwapChain() {
    auto extent = lveWindow.getExtent();
    // Gestione della minimizzazione della finestra: finché una delle dimensioni è 0,
    // mettiamo in pausa il programma e attendiamo nuovi eventi GLFW
    while (extent.width == 0 || extent.height == 0) {
      extent = lveWindow.getExtent();
      glfwWaitEvents();
    }

    // Attende che la GPU abbia terminato l'esecuzione dei comandi prima di distruggere o ricreare la swap chain
    vkDeviceWaitIdle(lveDevice.device());

    if (lveSwapChain == nullptr) {
      lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extent);
    } else {
      // Ricrea la swap chain passando la precedente (tramite std::move) per agevolare il riuso delle risorse
      lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extent, std::move(lveSwapChain));
      // Se il numero di immagini della nuova swap chain differisce, riallochiamo i command buffer
      if (lveSwapChain->imageCount() != commandBuffers.size()) {
        freeCommandBuffers();
        createCommandBuffers();
      }
    }

    // Ricrea la pipeline (al momento necessaria finché non verificheremo la piena compatibilità dei render pass)
    createPipeline();
  }

  void FirstApp::createCommandBuffers() {
    // In Vulkan i comandi di disegno non vengono eseguiti direttamente tramite chiamate a funzioni,
    // ma vengono registrati in un command buffer e poi sottomessi alla coda del device.
    // Creiamo un command buffer per ciascuna immagine della swap chain (relazione 1:1 con i
    // framebuffer), registrandoli una sola volta all'avvio per poi riutilizzarli a ogni frame.
    commandBuffers.resize(lveSwapChain->imageCount());

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

  void FirstApp::freeCommandBuffers() {
    // Rilascia la memoria di tutti i command buffer precedentemente allocati nel command pool
    vkFreeCommandBuffers(
      lveDevice.device(),
      lveDevice.getCommandPool(),
      static_cast<uint32_t>(commandBuffers.size()),
      commandBuffers.data());

    commandBuffers.clear();
  }

  void FirstApp::drawFrame() {
    uint32_t imageIndex;
    // Ottiene l'indice della prossima immagine disponibile nella swap chain su cui renderizzare.
    // Gestisce automaticamente la sincronizzazione CPU/GPU (fences e semafori) per double/triple buffering.
    auto result = lveSwapChain->acquireNextImage(&imageIndex);

    // Se la swap chain non è più valida (es. resize immediato della finestra), la ricrea ed esce dal frame
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapChain();
      return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Registra i comandi di rendering per l'immagine corrente prima della sottomissione
    recordCommandBuffer(imageIndex);

    // Invia il command buffer corrispondente alla graphics queue del device ed esegue il comando.
    // La swap chain presenterà poi a schermo l'immagine renderizzata al momento opportuno (in base al present mode).
    result = lveSwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);

    // Se la superficie è cambiata, è subottimale oppure è stato intercettato l'evento di resize dalla callback GLFW,
    // resettiamo il flag e ricreiamo la swap chain
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || lveWindow.wasWindowResized()) {
      lveWindow.resetWindowResizedFlag();
      recreateSwapChain();
      return;
    }

    if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
    }
  }

  void FirstApp::recordCommandBuffer(int imageIndex) {
    // Contatore per simulare un'animazione incrementale variando l'offset orizzontale a ogni frame registrato
    static int frame = 0;
    frame = (frame + 1) % 100;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
    }

    // Configurazione dell'inizio del Render Pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = lveSwapChain->getRenderPass();
    renderPassInfo.framebuffer = lveSwapChain->getFrameBuffer(imageIndex);

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
    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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
    vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
    vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

    // Associa la pipeline grafica, poi associa il vertex buffer del modello ed esegue il draw
    lvePipeline->bind(commandBuffers[imageIndex]);
    lveModel->bind(commandBuffers[imageIndex]);

    // Disegna 4 copie dello stesso modello semplicemente aggiornando i dati di push constants
    // (offset per la traslazione e colore) prima di ciascuna draw call
    for (int j = 0; j < 4; j++) {
      SimplePushConstantData push{};
      push.offset = { -0.5f + frame * 0.02f, -0.4f + j * 0.25f };
      push.color = { 0.0f, 0.0f, 0.2f + 0.2f * j };
      // Invia i dati delle push constants al command buffer per gli stadi Vertex e Fragment
      vkCmdPushConstants(
        commandBuffers[imageIndex],
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(SimplePushConstantData),
        &push);
      lveModel->draw(commandBuffers[imageIndex]);
    }

    // Termina il render pass e conclude la registrazione del command buffer
    vkCmdEndRenderPass(commandBuffers[imageIndex]);
    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
  }
} // namespace lve