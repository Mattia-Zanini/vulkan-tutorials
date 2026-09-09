#include "first_app.hpp"

#include "glm/common.hpp"
#include "keyboard_movement_controller.hpp"
#include "lve_camera.hpp"
#include "lve_game_object.hpp"
#include "lve_model.hpp"
#include "simple_render_system.hpp"
#include "vulkan/vulkan_core.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <chrono>
#include <memory>
#include <utility>
#include <vector>

#define MAX_FRAME_TIME (float)0.0166666667

namespace lve {

  FirstApp::FirstApp() { loadGameObjects(); }

  FirstApp::~FirstApp() {}

  void FirstApp::run() {
    // Sistema di rendering: incapsula pipeline, layout e logica di disegno per i game object,
    // configurandosi con il render pass fornito dal renderer
    SimpleRenderSystem simpleRenderSystem{ lveDevice, lveRenderer.getSwapChainRenderPass() };
    // Camera: memorizza la matrice di proiezione (ortografica o prospettica)
    LveCamera camera{};

    // Oggetto invisibile usato per mantenere lo stato della telecamera (posizione, orientamento)
    // dal momento che la classe LveCamera non memorizza internamente questi dati tra un frame e l'altro
    auto viewerObject = LveGameObject::createGameObject();
    KeyboardMovementController cameraController{};

    // Inizializza il timer usando chrono per avere una precisione elevata (necessario per calcolare il dt)
    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!lveWindow.shouldClose()) {
      glfwPollEvents();

      // Calcola il "time step" (o delta time / dt) misurando quanto tempo è trascorso
      // dall'ultima iterazione. Questo serve a separare la logica del gioco (es. movimento)
      // dal framerate (es. un cubo girerebbe più velocemente a 120fps che a 60fps senza dt).
      auto newTime = std::chrono::high_resolution_clock::now();
      float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
      currentTime = newTime;

      // Limita il frame time massimo per evitare scatti eccessivi nel caso in cui il programma
      // venga bloccato temporaneamente (ad esempio, se l'utente ridimensiona la finestra).
      frameTime = glm::min(frameTime, MAX_FRAME_TIME);

      // Aggiorna lo stato del viewerObject in base all'input e al dt
      cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);
      // Aggiorna infine la camera vera e propria leggendo la posizione e la rotazione del viewerObject
      camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

      // Calcola l'aspect ratio corrente della finestra/swap chain per compensare le distorsioni dovute al ridimensionamento
      float aspect = lveRenderer.getAspectRatio();

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

  void FirstApp::loadGameObjects() {
    // Carica il modello 3D leggendo le coordinate, indici e colori dal file .obj fornito,
    // usando la libreria tinyobjloader integrata nel Builder.
    std::shared_ptr<LveModel> lveModel = LveModel::createModelFromFile(lveDevice, "models/smooth_vase.obj");

    // Crea un game object per rappresentare il modello appena caricato
    auto gameObj = LveGameObject::createGameObject();
    gameObj.model = lveModel;
    // Con la proiezione prospettica, gli oggetti a valori di Z maggiori appaiono più lontani e rimpiccioliti.
    // Posizioniamo il vaso un po' più lontano (Z=2.5) e lo ingrandiamo di 3 volte per renderlo visibile,
    // mantenendolo nel frustum visivo tra near (0.1) e far (10.0).
    gameObj.transform.translation = { .0f, .0f, 2.5f };
    gameObj.transform.scale = glm::vec3{ 3.f };
    gameObjects.push_back(std::move(gameObj));
  }

} // namespace lve