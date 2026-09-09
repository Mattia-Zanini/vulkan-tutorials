#pragma once

#include "lve_game_object.hpp"

namespace lve {
  class KeyboardMovementController {
  public:
    // Struttura per mappare i tasti della tastiera (tramite codici GLFW) alle diverse azioni di movimento.
    // I valori di default possono essere modificati a runtime.
    struct KeyMappings {
      int moveLeft = GLFW_KEY_A;
      int moveRight = GLFW_KEY_D;
      int moveForward = GLFW_KEY_W;
      int moveBackward = GLFW_KEY_S;
      int moveUp = GLFW_KEY_E;
      int moveDown = GLFW_KEY_Q;
      int lookLeft = GLFW_KEY_LEFT;
      int lookRight = GLFW_KEY_RIGHT;
      int lookUp = GLFW_KEY_UP;
      int lookDown = GLFW_KEY_DOWN;
    };

    // Muove un GameObject con i controlli relativi alla direzione verso cui l'oggetto è rivolto,
    // muovendolo all'interno del piano XZ. Richiede un puntatore alla finestra GLFW per leggere
    // l'input, il delta time (dt) per slegare il movimento dal framerate e l'oggetto bersaglio.
    void moveInPlaneXZ(GLFWwindow* window, float dt, LveGameObject& gameObject);

    KeyMappings keys{};
    float moveSpeed{ 3.f };
    float lookSpeed{ 1.5f };
  };
} // namespace lve