#pragma once

#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_pipeline.hpp"

// std
#include <memory>

namespace lve {
  // PointLightSystem: sistema di rendering secondario e indipendente dedicato a visualizzare
  // le sorgenti di luce puntiformi come billboard 2D sempre orientati verso la telecamera (screen-aligned billboards).
  class PointLightSystem {
  public:
    PointLightSystem(
      LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~PointLightSystem();

    PointLightSystem(const PointLightSystem&) = delete;
    PointLightSystem& operator=(const PointLightSystem&) = delete;

    // Aggiorna l'array di point light nell'UBO globale copiando le proprietà dai rispettivi game object attivi
    void update(FrameInfo& frameInfo, GlobalUbo& ubo);
    // Registra i comandi di disegno per i billboard delle point light
    void render(FrameInfo& frameInfo);

  private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkRenderPass renderPass);

    LveDevice& lveDevice;

    std::unique_ptr<LvePipeline> lvePipeline;
    VkPipelineLayout pipelineLayout;
  };
} // namespace lve