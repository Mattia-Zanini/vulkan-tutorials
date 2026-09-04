#pragma once

#include "lve_window.hpp"
#include "lve_pipeline.hpp"
#include "lve_device.hpp"
#include "lve_swap_chain.hpp"
#include "vulkan/vulkan_core.h"

// std
#include <memory>
#include <vector>

namespace lve {
  class FirstApp {
  public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    FirstApp();
    ~FirstApp();

    // Gestione del ciclo di vita: FirstApp gestisce risorse Vulkan esplicite (pipelineLayout,
    // command buffers), pertanto eliminiamo costruttore di copia e operatore di assegnazione.
    FirstApp(const FirstApp&) = delete;
    FirstApp& operator=(const FirstApp&) = delete;

    void run();

  private:
    void createPipelineLayout();
    void createPipeline();
    void createCommandBuffers();
    void drawFrame();

    // L'ordine di dichiarazione definisce l'ordine di inizializzazione (dall'alto in basso)
    // e di distruzione (in ordine inverso, dal basso in alto). Questo ordine è critico:
    // la swap chain dipende da device e window, e la pipeline dipende dal render pass della swap
    // chain.
    LveWindow lveWindow{ WIDTH, HEIGHT, "Hello Vulkan!" };
    LveDevice lveDevice{ lveWindow };
    // Swap chain: gestisce i frame buffer multipli (double/triple buffering) e sincronizza la
    // presentazione a schermo
    LveSwapChain lveSwapChain{ lveDevice, lveWindow.getExtent() };

    // Smart pointer (unique_ptr): gestisce automaticamente la memoria della pipeline senza dover
    // chiamare manualmente delete
    std::unique_ptr<LvePipeline> lvePipeline;
    // Pipeline Layout: descrive le risorse esterne (come descriptor set e push constants)
    // accessibili dagli shader
    VkPipelineLayout pipelineLayout;
    // Command Buffer: registrano i comandi di rendering da inviare alla GPU
    std::vector<VkCommandBuffer> commandBuffer;
  };
}