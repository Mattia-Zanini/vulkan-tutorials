#pragma once

#include "lve_camera.hpp"
#include "lve_game_object.hpp"
#include "vulkan/vulkan_core.h"

// lib
#include <vulkan/vulkan.h>

namespace lve {

  // Incapsula tutte le informazioni rilevanti per il frame corrente (indice, delta time, command buffer e camera).
  // Raggruppare questi dati in un unico oggetto evita di dover aggiornare la firma di tutti i metodi
  // dei sottosistemi di rendering ogni volta che viene aggiunto un nuovo parametro per-frame.
  struct FrameInfo {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    LveCamera& camera;
    VkDescriptorSet globalDescriptoSet; // Descriptor set globale per il frame corrente (contiene l'UBO)
    LveGameObject::Map& gameObjects;    // Riferimento a tutti i game object attivi nella scena
  };

} // namespace lve