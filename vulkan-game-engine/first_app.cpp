#include "first_app.hpp"

#include "glm/common.hpp"
#include "keyboard_movement_controller.hpp"
#include "lve_buffer.hpp"
#include "lve_camera.hpp"
#include "lve_frame_info.hpp"
#include "lve_game_object.hpp"
#include "lve_model.hpp"
#include "lve_swap_chain.hpp"
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

  // Uniform Buffer Object (UBO) globale: permette di passare dati arbitrari in sola lettura agli shader
  // superando i limiti di dimensione delle push constants (128 byte garantiti vs almeno 16KB per gli UBO).
  struct GlobalUbo {
    glm::mat4 projectionView{ 1.f };
    glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.f, -3.f, -1.f });
  };

  FirstApp::FirstApp() { loadGameObjects(); }

  FirstApp::~FirstApp() {}

  void FirstApp::run() {
    // Alloca un buffer UBO separato per ciascun frame in flight (MAX_FRAMES_IN_FLIGHT).
    // Separare i buffer per ciascun frame (con instanceCount = 1) evita problemi di allineamento
    // con il limite hardware 'nonCoherentAtomSize' del dispositivo, che altrimenti si verificherebbero
    // compattando più istanze in un unico buffer ed eseguendo il flush di un singolo indice.
    std::vector<std::unique_ptr<LveBuffer>> uboBuffers(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
      uboBuffers[i] = std::make_unique<LveBuffer>(
        lveDevice,
        sizeof(GlobalUbo),
        1,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      uboBuffers[i]->map();
    }

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
        int frameIndex = lveRenderer.getFrameIndex();
        // Raggruppa i parametri del frame corrente in una singola struct da passare ai render systems
        FrameInfo frameInfo{ frameIndex, frameTime, commandBuffer, camera };

        // Fase 1: Aggiornamento degli oggetti e della memoria
        GlobalUbo ubo{};
        ubo.projectionView = camera.getProjection() * camera.getView();
        // Scrive i dati e ne esegue il flush sul buffer UBO dedicato al frame corrente
        uboBuffers[frameIndex]->writeToBuffer(&ubo);
        uboBuffers[frameIndex]->flush();

        // Fase 2: Registrazione dei comandi di rendering
        lveRenderer.beginSwapChainRenderPass(commandBuffer);
        simpleRenderSystem.renderGameObjects(frameInfo, gameObjects);
        lveRenderer.endSwapChainRenderPass(commandBuffer);
        lveRenderer.endFrame();
      }
    }

    // Attende che la GPU abbia completato tutte le operazioni prima di uscire dall'applicazione.
    // Evita errori e messaggi dai validation layers dovuti alla distruzione di risorse ancora in
    // uso dalla GPU.
    vkDeviceWaitIdle(lveDevice.device());
  }

  void FirstApp::loadGameObjects() {
    // Carica il modello con shading piatto (flat shading / face normals: ciascuna faccia ha normali distinte)
    std::shared_ptr<LveModel> lveModel = LveModel::createModelFromFile(lveDevice, "models/flat_vase.obj");
    auto flatVase = LveGameObject::createGameObject();
    flatVase.model = lveModel;
    flatVase.transform.translation = { -.5f, .5f, 2.5f };
    // Scala non uniforme lungo Y (1.5 rispetto a 3.0 su X e Z) per testare la correttezza della normalMatrix
    flatVase.transform.scale = glm::vec3{ 3.f, 1.5f, 3.f };
    gameObjects.push_back(std::move(flatVase));

    // Carica lo stesso modello con shading liscio (smooth shading / vertex normals: normali interpolate sulla superficie)
    lveModel = LveModel::createModelFromFile(lveDevice, "models/smooth_vase.obj");
    auto smoothVase = LveGameObject::createGameObject();
    smoothVase.model = lveModel;
    smoothVase.transform.translation = { .5f, .5f, 2.5f };
    smoothVase.transform.scale = glm::vec3{ 3.f, 1.5f, 3.f };
    gameObjects.push_back(std::move(smoothVase));
  }

} // namespace lve