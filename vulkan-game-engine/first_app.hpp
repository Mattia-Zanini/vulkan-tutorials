#pragma once

#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_game_object.hpp"
#include "lve_renderer.hpp"
#include "lve_window.hpp"

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
    // Inizializza i game object (modelli, trasformazioni, colori) dell'applicazione
    void loadGameObjects();

    // L'ordine di dichiarazione definisce l'ordine di inizializzazione (dall'alto in basso)
    // e di distruzione (in ordine inverso, dal basso in alto). Questo ordine è critico:
    // la swap chain dipende da device e window, e la pipeline dipende dal render pass della swap
    // chain.
    LveWindow lveWindow{ WIDTH, HEIGHT, "Hello Vulkan!" };
    LveDevice lveDevice{ lveWindow };
    // Renderer: incapsula la gestione della swap chain, dei command buffer e del ciclo di rendering del frame
    LveRenderer lveRenderer{ lveWindow, lveDevice };

    // Pool globale per allocare i descriptor set condivisi tra più sistemi (es. UBO globale).
    // Dichiarato dopo lveDevice in modo da essere distrutto prima del device stesso.
    std::unique_ptr<LveDescriptorPool> globalPool{};
    // Collezione dei Game Object presenti nella scena (ciascuno dotato di modello, trasformazione 2D e colore)
    std::vector<LveGameObject> gameObjects;
  };
}