#include "first_app.hpp"

#include "simple_render_system.hpp"
#include "vulkan/vulkan_core.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <memory>
#include <utility>
#include <vector>

namespace lve {

  FirstApp::FirstApp() {
    // Inizializza le risorse Vulkan necessarie: modelli, layout della pipeline, swap chain e command buffers
    loadGameObjects();
  }

  FirstApp::~FirstApp() {}

  void FirstApp::run() {
    // Sistema di rendering: incapsula pipeline, layout e logica di disegno per i game object,
    // configurandosi con il render pass fornito dal renderer
    SimpleRenderSystem simpleRenderSystem{ lveDevice, lveRenderer.getSwapChainRenderPass() };

    while (!lveWindow.shouldClose()) {
      glfwPollEvents();

      // beginFrame restituisce nullptr se la swap chain viene ricreata (es. resize della finestra);
      // in tal caso saltiamo la registrazione e sottomissione dei comandi per questo frame
      if (auto commandBuffer = lveRenderer.beginFrame()) {
        lveRenderer.beginSwapChainRenderPass(commandBuffer);
        simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
        lveRenderer.endSwapChainRenderPass(commandBuffer);
        lveRenderer.endFrame();
      }
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

  void FirstApp::loadGameObjects() {
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

    // Alloca il vertex buffer sulla GPU e copia i dati dei vertici tramite LveModel.
    // Usiamo uno shared_ptr in modo che più entità possano referenziare e condividere la stessa geometria.
    auto lveModel = std::make_shared<LveModel>(lveDevice, vertices);

    // Palette di colori in spazio sRGB (tratta da https://www.color-hex.com/color-palette/5361)
    std::vector<glm::vec3> colors{
      { 1.0f, 0.7f, 0.73f },
      { 1.0f, 0.87f, 0.73f },
      { 1.0f, 1.0f, 0.73f },
      { 0.73f, 1.0f, 0.8f },
      { 0.73, 0.88f, 1.0f }
    };

    // Conversione da spazio sRGB a lineare (gamma correction con esponente 2.2):
    // i valori RGB definiti dall'utente o dal web sono solitamente in sRGB; convertendoli in spazio lineare
    // la swap chain (che utilizza un formato sRGB) riapplicherà la curva corretta evitando colori slavati o troppo chiari
    for (auto& color : colors) {
      color = glm::pow(color, glm::vec3{ 2.2f });
    }

    // Istanziazione di 40 game object che condividono lo stesso modello geometrico di base (lveModel).
    // NOTA SULL'ORDINE VISIVO E IL DEPTH BUFFER:
    // Anche se lavoriamo in 2D e tutti i triangoli hanno z = 0 nello shader, il triangolo più piccolo (i = 0)
    // appare sempre "in cima" e non viene coperto dai triangoli più grandi successivi (i > 0).
    // Questo accade perché:
    // 1) gameObjects[0] (il più piccolo) viene disegnato per primo e scrive la sua profondità (0.0) nel Depth Buffer.
    // 2) La pipeline grafica è configurata con VK_COMPARE_OP_LESS per il Depth Test.
    // 3) Quando i triangoli più grandi successivi provano a colorare i pixel centrali, il loro test di profondità
    //    diventa (0.0 < 0.0), che è FALSO: di conseguenza i pixel sovrapposti vengono scartati dalla GPU,
    //    preservando il triangolo iniziale e disegnando solo le porzioni esterne "non ancora occupate".
    for (int i = 0; i < 40; i++) {
      // Creiamo una nuova entità (LveGameObject) tramite il factory method
      auto triangle = LveGameObject::createGameObject();
      triangle.model = lveModel;                                     // Assegna il modello condiviso (riuso del vertex buffer sulla GPU)
      triangle.color = colors[i % colors.size()];                    // Alterna ciclicamente i colori della palette
      triangle.transform2d.translation.x = 0.0f;                     // Traslazione orizzontale fissa
      triangle.transform2d.scale = glm::vec2(0.5f) + i * 0.025f;     // Scala progressivamente crescente per ogni triangolo
      triangle.transform2d.rotation = i * glm::pi<float>() * 0.025f; // Rotazione iniziale sfasata

      // Aggiunge il game object al vettore trasferendone la proprietà tramite std::move (LveGameObject non è copiabile)
      gameObjects.push_back(std::move(triangle));
    }
  }

} // namespace lve