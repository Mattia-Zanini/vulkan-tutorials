#include "systems/simple_render_system.hpp"
#include "lve_game_object.hpp"
#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <cstdint>

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
    glm::mat4 modelMatrix{ 1.f };
    // Matrice delle normali passata come mat4 (anziché mat3) per rispettare le regole di allineamento
    // di Vulkan (dove ogni riga/colonna richiede un allineamento a 16 byte); GLM gestisce automaticamente il padding
    glm::mat4 normalMatrix{ 1.f };
  };

  SimpleRenderSystem::SimpleRenderSystem(LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
    : lveDevice{ device } {
    // Inizializza il layout delle push constants e crea la pipeline grafica associata al render pass specificato
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
  }

  SimpleRenderSystem::~SimpleRenderSystem() {
    // Distruzione esplicita del layout della pipeline al termine del ciclo di vita del render system
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
  }

  void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    // Configura il range di push constants: specifica quali stadi dello shader possono accedervi,
    // l'offset di partenza (0) e la dimensione totale occupata in byte.
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

    // Pipeline Layout: definisce come passare dati agli shader oltre ai dati dei vertici.
    // Include descrittori (texture, Uniform Buffer Objects) e push constants.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // Specifica il numero e i puntatori ai descriptor set layout attesi dalla pipeline (set 0, set 1, ecc.)
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
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

  void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
    // Esegue il bind della pipeline una sola volta per tutti gli oggetti che condividono lo stesso stato di rendering
    lvePipeline->bind(frameInfo.commandBuffer);

    // Esegue il bind del descriptor set globale (set 0) una sola volta all'esterno del ciclo degli oggetti.
    // Essendo condiviso da tutti gli oggetti della scena nel frame corrente, non serve ri-eseguire il bind a ogni draw call.
    vkCmdBindDescriptorSets(
      frameInfo.commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipelineLayout,
      0,
      1,
      &frameInfo.globalDescriptorSet,
      0,
      nullptr);

    // Itera attraverso la mappa dei game object (coppie chiave-valore ID -> GameObject)
    for (auto& kv : frameInfo.gameObjects) {
      auto& obj = kv.second;
      // Criterio di filtraggio: renderizza solo gli oggetti che possiedono un modello 3D associato
      if (obj.model == nullptr)
        continue;

      // Prepara i dati delle push constants specifici per questo oggetto
      SimplePushConstantData push{};

      // Matrice di trasformazione del modello (Model matrix): non serve più moltiplicarla per projectionView
      // qui sulla CPU, poiché projectionView viene letta direttamente dall'UBO globale nello shader
      push.modelMatrix = obj.transform.mat4();
      // Assegna la matrice delle normali calcolata per trasformare le normali nello spazio mondo
      push.normalMatrix = obj.transform.normalMatrix();

      // Invia i dati delle push constants alla GPU prima del disegno dell'oggetto
      vkCmdPushConstants(
        frameInfo.commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(SimplePushConstantData),
        &push);

      // Collega il vertex buffer del modello ed emette il comando di disegno
      obj.model->bind(frameInfo.commandBuffer);
      obj.model->draw(frameInfo.commandBuffer);
    }
  }

} // namespace lve