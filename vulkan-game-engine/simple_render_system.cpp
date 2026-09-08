#include "simple_render_system.hpp"
#include "lve_game_object.hpp"
#include "vulkan/vulkan_core.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <memory>
#include <stdexcept>
#include <vector>

namespace lve {

  // Struttura dati per le Push Constants: permette di passare piccoli blocchi di dati ai vari stadi dello shader
  // direttamente tramite il command buffer (senza allocazioni di memoria o descrittori).
  // Lo standard Vulkan garantisce almeno 128 byte condivisi tra tutti gli stadi.
  struct SimplePushConstantData {
    // Matrice di trasformazione affine 4x4 (combina scala, rotazione ed offset/traslazione tramite coordinate omogenee).
    // Inizializzata di default alla matrice identità.
    glm::mat4 transform{ 1.f };
    // In memoria GPU (regole di allineamento std430/std140), un vec3 deve essere allineato a un multiplo di 16 byte (4N).
    // Usiamo alignas(16) per forzare lo stesso padding di 8 byte anche nella struct host C++, evitando disallineamenti di lettura.
    alignas(16) glm::vec3 color;
  };

  SimpleRenderSystem::SimpleRenderSystem(LveDevice& device, VkRenderPass renderPass) : lveDevice{ device } {
    // Inizializza il layout delle push constants e crea la pipeline grafica associata al render pass specificato
    createPipelineLayout();
    createPipeline(renderPass);
  }

  SimpleRenderSystem::~SimpleRenderSystem() {
    // Distruzione esplicita del layout della pipeline al termine del ciclo di vita del render system
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
  }

  void SimpleRenderSystem::createPipelineLayout() {
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

  void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    // Utilizziamo le dimensioni (width/height) della swap chain e non della finestra:
    // su display ad alta densità (es. Retina su macOS), le coordinate finestra differiscono dal
    // numero reale di pixel.
    LvePipeline::defaultPipelineConfigInfo(pipelineConfig);

    // Il render pass funge da "blueprint" che descrive la struttura del framebuffer (attachment di
    // colore, depth, ecc.) La pipeline deve sapere in anticipo con quale layout di render pass sarà
    // compatibile per produrre l'output corretto.
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;

    // Creiamo l'istanza della pipeline mediante unique_ptr
    lvePipeline = std::make_unique<LvePipeline>(lveDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);
  }

  void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<LveGameObject>& gameObjects) {
    // Esegue il bind della pipeline una sola volta per tutti gli oggetti che condividono lo stesso stato di rendering
    lvePipeline->bind(commandBuffer);

    for (auto& obj : gameObjects) {
      // Aggiorna continuamente le componenti di rotazione per animare l'oggetto:
      // rotazione principale attorno all'asse Y (verticale) e rotazione secondaria attorno all'asse X a metà velocità
      obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.01f, glm::two_pi<float>());
      obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.005f, glm::two_pi<float>());

      // Prepara i dati delle push constants specifici per questo oggetto
      SimplePushConstantData push{};
      push.color = obj.color;
      // Calcola la matrice di trasformazione affine 4x4 combinata (Translate * Ry * Rx * Rz * Scale)
      push.transform = obj.transform.mat4();

      // Invia i dati delle push constants alla GPU prima del disegno dell'oggetto
      vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(SimplePushConstantData),
        &push);

      // Collega il vertex buffer del modello ed emette il comando di disegno
      obj.model->bind(commandBuffer);
      obj.model->draw(commandBuffer);
    }
  }
} // namespace lve