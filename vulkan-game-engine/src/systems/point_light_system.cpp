#include "systems/point_light_system.hpp"
#include "vulkan/vulkan_core.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <cassert>
#include <stdexcept>

namespace lve {

  // Push constants per il rendering dei billboard: trasmettono posizione, colore/intensità e raggio
  // specifici di ogni singola point light senza richiedere l'indicizzazione nell'UBO
  struct PointLightPushConstants {
    glm::vec4 position{};
    glm::vec4 color{};
    float radius;
  };

  PointLightSystem::PointLightSystem(LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
    : lveDevice{ device } {
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
  }

  PointLightSystem::~PointLightSystem() {
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
  }

  void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    // Configura il push constant range per rendere disponibili i dati della point light sia al vertex che al fragment shader
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PointLightPushConstants);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
    }
  }

  void PointLightSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    LvePipeline::defaultPipelineConfigInfo(pipelineConfig);
    // Questo sistema non fa uso di vertex buffer: i vertici del billboard sono generati nello shader.
    // Svuotiamo le descrizioni per evitare warning dai validation layers.
    pipelineConfig.attributeDescriptions.clear();
    pipelineConfig.bindingDescriptions.clear();
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;

    lvePipeline = std::make_unique<LvePipeline>(
      lveDevice,
      "shaders/point_light.vert.spv",
      "shaders/point_light.frag.spv",
      pipelineConfig);
  }

  void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo) {
    // Matrice di rotazione per animare le luci attorno all'asse Y in proporzione al tempo trascorso (frameTime)
    auto rotateLight = glm::rotate(glm::mat4(1.f), frameInfo.frameTime, { 0.f, -1.f, 0.f });

    int lightIndex = 0;
    for (auto& kv : frameInfo.gameObjects) {
      auto& obj = kv.second;
      // Salta i game object che non sono point light
      if (obj.pointLight == nullptr)
        continue;

      // Verifica che non sia stato superato il numero massimo di luci allocato nell'array dell'UBO
      assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");

      // Aggiorna la posizione della luce ruotandola attorno all'origine
      obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));

      // Copia posizione (convertita in vec4 con w=1) e colore/intensità (RGB + intensity in w) nell'UBO
      ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.f);
      ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
      lightIndex += 1;
    }

    // Salva il numero effettivo di point light trovate nella scena
    ubo.numLights = lightIndex;
  }

  void PointLightSystem::render(FrameInfo& frameInfo) {
    lvePipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(
      frameInfo.commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipelineLayout,
      0,
      1,
      &frameInfo.globalDescriptorSet,
      0,
      nullptr);

    // Itera tutti i game object ed effettua una draw call con push constants per ciascuna point light attiva
    for (auto& kv : frameInfo.gameObjects) {
      auto& obj = kv.second;
      if (obj.pointLight == nullptr)
        continue;

      PointLightPushConstants push{};
      push.position = glm::vec4(obj.transform.translation, 1.f);
      push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
      push.radius = obj.transform.scale.x;

      vkCmdPushConstants(
        frameInfo.commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PointLightPushConstants),
        &push);

      // Emette un draw call per 6 vertici (2 triangoli per il quad del billboard) senza associare vertex buffer
      vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
    }
  }

} // namespace lve