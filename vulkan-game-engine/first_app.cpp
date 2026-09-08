#include "first_app.hpp"

#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"
#include "lve_camera.hpp"
#include "lve_model.hpp"
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

  FirstApp::FirstApp() { loadGameObjects(); }

  FirstApp::~FirstApp() {}

  void FirstApp::run() {
    // Sistema di rendering: incapsula pipeline, layout e logica di disegno per i game object,
    // configurandosi con il render pass fornito dal renderer
    SimpleRenderSystem simpleRenderSystem{ lveDevice, lveRenderer.getSwapChainRenderPass() };
    // Camera: memorizza la matrice di proiezione (ortografica o prospettica)
    LveCamera camera{};
    // Imposta la vista specificando la direzione o un punto bersaglio (target)
    // In questo caso, posiziona la telecamera in (-1, -2, 2) e la punta verso (0, 0, 1.5)
    // camera.setViewDirection(glm::vec3{ 0.f }, glm::vec3{ 0.5f, 0.f, 1.f });
    camera.setViewTarget(glm::vec3{ -1.f, -2.f, 2.f }, glm::vec3{ 0.f, 0.f, 1.5f });

    while (!lveWindow.shouldClose()) {
      glfwPollEvents();

      // Calcola l'aspect ratio corrente della finestra/swap chain per compensare le distorsioni dovute al ridimensionamento
      float aspect = lveRenderer.getAspectRatio();
      // Proiezione ortografica alternativa (decommentabile per test):
      // camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);

      // Configura la matrice di proiezione prospettica:
      // fovy: campo visivo verticale (Field of View) in radianti (50 gradi)
      // aspect: rapporto di aspetto larghezza/altezza
      // near e far: piani di clipping vicino (0.1) e lontano (10.0)
      camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);

      // beginFrame restituisce nullptr se la swap chain viene ricreata (es. resize della finestra);
      // in tal caso saltiamo la registrazione e sottomissione dei comandi per questo frame
      if (auto commandBuffer = lveRenderer.beginFrame()) {
        lveRenderer.beginSwapChainRenderPass(commandBuffer);
        simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
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

  // Funzione helper temporanea: genera la geometria di un cubo 1x1x1 centrato rispetto a un offset specificato,
  // con colori per-vertice distinti per ciascuna delle sei facce (left, right, top, bottom, nose, tail)
  std::unique_ptr<LveModel> createCubeModel(LveDevice& device, glm::vec3 offset) {
    std::vector<LveModel::Vertex> vertices{

      // left face (white)
      { { -.5f, -.5f, -.5f }, { .9f, .9f, .9f } },
      { { -.5f, .5f, .5f }, { .9f, .9f, .9f } },
      { { -.5f, -.5f, .5f }, { .9f, .9f, .9f } },
      { { -.5f, -.5f, -.5f }, { .9f, .9f, .9f } },
      { { -.5f, .5f, -.5f }, { .9f, .9f, .9f } },
      { { -.5f, .5f, .5f }, { .9f, .9f, .9f } },

      // right face (yellow)
      { { .5f, -.5f, -.5f }, { .8f, .8f, .1f } },
      { { .5f, .5f, .5f }, { .8f, .8f, .1f } },
      { { .5f, -.5f, .5f }, { .8f, .8f, .1f } },
      { { .5f, -.5f, -.5f }, { .8f, .8f, .1f } },
      { { .5f, .5f, -.5f }, { .8f, .8f, .1f } },
      { { .5f, .5f, .5f }, { .8f, .8f, .1f } },

      // top face (orange, remember y axis points down)
      { { -.5f, -.5f, -.5f }, { .9f, .6f, .1f } },
      { { .5f, -.5f, .5f }, { .9f, .6f, .1f } },
      { { -.5f, -.5f, .5f }, { .9f, .6f, .1f } },
      { { -.5f, -.5f, -.5f }, { .9f, .6f, .1f } },
      { { .5f, -.5f, -.5f }, { .9f, .6f, .1f } },
      { { .5f, -.5f, .5f }, { .9f, .6f, .1f } },

      // bottom face (red)
      { { -.5f, .5f, -.5f }, { .8f, .1f, .1f } },
      { { .5f, .5f, .5f }, { .8f, .1f, .1f } },
      { { -.5f, .5f, .5f }, { .8f, .1f, .1f } },
      { { -.5f, .5f, -.5f }, { .8f, .1f, .1f } },
      { { .5f, .5f, -.5f }, { .8f, .1f, .1f } },
      { { .5f, .5f, .5f }, { .8f, .1f, .1f } },

      // nose face (blue)
      { { -.5f, -.5f, 0.5f }, { .1f, .1f, .8f } },
      { { .5f, .5f, 0.5f }, { .1f, .1f, .8f } },
      { { -.5f, .5f, 0.5f }, { .1f, .1f, .8f } },
      { { -.5f, -.5f, 0.5f }, { .1f, .1f, .8f } },
      { { .5f, -.5f, 0.5f }, { .1f, .1f, .8f } },
      { { .5f, .5f, 0.5f }, { .1f, .1f, .8f } },

      // tail face (green)
      { { -.5f, -.5f, -0.5f }, { .1f, .8f, .1f } },
      { { .5f, .5f, -0.5f }, { .1f, .8f, .1f } },
      { { -.5f, .5f, -0.5f }, { .1f, .8f, .1f } },
      { { -.5f, -.5f, -0.5f }, { .1f, .8f, .1f } },
      { { .5f, -.5f, -0.5f }, { .1f, .8f, .1f } },
      { { .5f, .5f, -0.5f }, { .1f, .8f, .1f } },

    };
    for (auto& v : vertices) {
      v.position += offset;
    }
    return std::make_unique<LveModel>(device, vertices);
  }

  void FirstApp::loadGameObjects() {
    // Crea il modello del cubo tramite la funzione helper e lo condivide come puntatore gestito
    std::shared_ptr<LveModel> lveModel = createCubeModel(lveDevice, { .0f, .0f, .0f });

    // Crea un game object per rappresentare il cubo 3D
    auto cube = LveGameObject::createGameObject();
    cube.model = lveModel;
    // Con la proiezione prospettica, gli oggetti a valori di Z maggiori appaiono più lontani e rimpiccioliti.
    // Posizionando il cubo a z = 1.5 (invece di 0.5), esso si trova a una distanza visiva confortevole all'interno
    // del frustum compreso tra near (0.1) e far (10.0), mantenendo all'incirca le dimensioni percepite in precedenza.
    cube.transform.translation = { .0f, .0f, 1.5f };
    cube.transform.scale = { .5f, .5f, .5f };
    gameObjects.push_back(std::move(cube));
  }

} // namespace lve