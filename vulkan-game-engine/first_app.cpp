#include "first_app.hpp"
#include "vulkan/vulkan_core.h"

namespace lve {

  FirstApp::FirstApp() {
    // Inizializza le risorse Vulkan necessarie: layout della pipeline, la pipeline grafica e i
    // command buffers
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
    }
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
    if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
    }
  }

  void FirstApp::createPipeline() {
    // Utilizziamo le dimensioni (width/height) della swap chain e non della finestra:
    // su display ad alta densità (es. Retina su macOS), le coordinate finestra differiscono dal
    // numero reale di pixel.
    auto pipelineConfig =
        LvePipeline::defaultPipelineConfigInfo(lveSwapChain.width(), lveSwapChain.height());

    // Il render pass funge da "blueprint" che descrive la struttura del framebuffer (attachment di
    // colore, depth, ecc.) La pipeline deve sapere in anticipo con quale layout di render pass sarà
    // compatibile per produrre l'output corretto.
    pipelineConfig.renderPass = lveSwapChain.getRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;

    // Creiamo l'istanza della pipeline mediante unique_ptr
    lvePipeline = std::make_unique<LvePipeline>(lveDevice, "shaders/simple_shader.vert.spv",
                                                "shaders/simple_shader.frag.spv", pipelineConfig);
  }
  void FirstApp::createCommandBuffers() {}
  void FirstApp::drawFrame() {}
} // namespace lve