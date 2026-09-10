#include "keyboard_movement_controller.hpp"

// std
#include <limits>

namespace lve {

  void KeyboardMovementController::moveInPlaneXZ(GLFWwindow* window, float dt, LveGameObject& gameObject) {
    glm::vec3 rotate{ 0 };
    // Raccoglie l'input di rotazione dalla tastiera (glfwGetKey controlla se il tasto è correntemente premuto)
    if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS)
      rotate.y += 1.f;
    if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS)
      rotate.y -= 1.f;
    if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS)
      rotate.x += 1.f;
    if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS)
      rotate.x -= 1.f;

    // Se l'input di rotazione è non nullo (verificato calcolando il prodotto scalare del vettore con se stesso
    // e confrontandolo con epsilon per evitare paragoni diretti float == 0):
    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
      // Normalizza il vettore per evitare rotazioni diagonali più veloci
      // e scala per la velocità e il delta time (dt) per rendere il movimento indipendente dal framerate
      gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);

    // Limita la rotazione sull'asse X (pitch) a circa +/- 85 gradi (1.5 rad) per impedire capovolgimenti completi
    gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
    // Usa il modulo su 2*pi per l'angolo Y (yaw) per evitare un overflow girando continuamente nella stessa direzione
    gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

    float yaw = gameObject.transform.rotation.y;
    // Calcola il vettore in avanti in base alla rotazione sull'asse Y
    const glm::vec3 forwardDir{ sin(yaw), 0.f, cos(yaw) };
    // Calcola il vettore a destra trovando il vettore perpendicolare sul piano XZ
    const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
    const glm::vec3 upDir{ 0.f, -1.f, 0.f };

    glm::vec3 moveDir{ 0.f };
    // Raccoglie l'input per la traslazione
    if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS)
      moveDir += forwardDir;
    if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS)
      moveDir -= forwardDir;
    if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS)
      moveDir += rightDir;
    if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS)
      moveDir -= rightDir;
    if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS)
      moveDir += upDir;
    if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS)
      moveDir -= upDir;

    // Se c'è un input di movimento, normalizzalo (per evitare che si vada più veloci in diagonale)
    // e applica l'aggiornamento alla posizione basato sul dt
    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
      gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
  }
} // namespace lve