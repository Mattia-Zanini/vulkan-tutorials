#pragma once

#include "lve_window.hpp"
#include "lve_pipeline.hpp"
#include "lve_device.hpp"
#include "lve_swap_chain.hpp"
#include "lve_model.hpp"

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
    void loadModels();
    void createPipelineLayout();
    void createPipeline();
    void createCommandBuffers();
    // Dealloca i command buffer dal command pool (usato quando cambia il numero di immagini della swap chain)
    void freeCommandBuffers();
    void drawFrame();
    // Ricrea la swap chain (e la pipeline dipendente) quando la finestra viene ridimensionata
    void recreateSwapChain();
    // Registra i comandi di rendering nel command buffer per ogni frame prima della sottomissione
    void recordCommandBuffer(int imageIndex);

    // L'ordine di dichiarazione definisce l'ordine di inizializzazione (dall'alto in basso)
    // e di distruzione (in ordine inverso, dal basso in alto). Questo ordine è critico:
    // la swap chain dipende da device e window, e la pipeline dipende dal render pass della swap
    // chain.
    LveWindow lveWindow{ WIDTH, HEIGHT, "Hello Vulkan!" };
    LveDevice lveDevice{ lveWindow };
    // Swap chain: gestita tramite unique_ptr per consentire la distruzione e ricreazione dinamica al resize della finestra
    std::unique_ptr<LveSwapChain> lveSwapChain;

    // Smart pointer (unique_ptr): gestisce automaticamente la memoria della pipeline senza dover
    // chiamare manualmente delete
    std::unique_ptr<LvePipeline> lvePipeline;
    // Pipeline Layout: descrive le risorse esterne (come descriptor set e push constants)
    // accessibili dagli shader
    VkPipelineLayout pipelineLayout;
    // Command Buffer: registrano i comandi di rendering da inviare alla GPU
    std::vector<VkCommandBuffer> commandBuffers;
    // Modello: contiene i dati geometrici dei vertici e il relativo vertex buffer allocato sulla GPU
    std::unique_ptr<LveModel> lveModel;
  };
}