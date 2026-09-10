#pragma once

#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_pipeline.hpp"
#include "vulkan/vulkan_core.h"

// std
#include <memory>

namespace lve {
  // SimpleRenderSystem: sistema di rendering responsabile della pipeline grafica,
  // del layout della pipeline e della registrazione dei comandi di disegno per i game object.
  // Più render system possono coesistere ed operare su sottoinsiemi diversi di componenti delle entità.
  class SimpleRenderSystem {
  public:
    SimpleRenderSystem(LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~SimpleRenderSystem();

    // Gestione del ciclo di vita: SimpleRenderSystem gestisce risorse Vulkan esplicite (pipelineLayout,
    // pipeline), pertanto eliminiamo costruttore di copia e operatore di assegnazione.
    SimpleRenderSystem(const SimpleRenderSystem&) = delete;
    SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

    // Registra i comandi di rendering per ciascun game object compatibile ricevendo il contesto del frame (FrameInfo)
    void renderGameObjects(FrameInfo& frameInfo);

  private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkRenderPass renderPass);

    LveDevice& lveDevice;

    // Smart pointer (unique_ptr): gestisce automaticamente la memoria della pipeline
    // senza dover chiamare manualmente delete
    std::unique_ptr<LvePipeline> lvePipeline;
    // Pipeline Layout: descrive le risorse esterne (come descriptor set e push constants)
    // accessibili dagli shader
    VkPipelineLayout pipelineLayout;
    // Layout del descriptor set specifico di questo sistema (Set 1: UBO per-oggetto e combined image sampler diffuseMap)
    std::unique_ptr<LveDescriptorSetLayout> renderSystemLayout;
  };
}