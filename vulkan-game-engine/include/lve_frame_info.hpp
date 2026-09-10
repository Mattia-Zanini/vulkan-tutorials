#pragma once

#include "lve_camera.hpp"
#include "lve_descriptors.hpp"
#include "lve_game_object.hpp"
#include "vulkan/vulkan_core.h"

// lib
#include <vulkan/vulkan.h>

namespace lve {

  // Numero massimo di point light gestibili nella scena per contenere il costo computazionale negli shader
#define MAX_LIGHTS 10

  // Dati di una singola point light: entrambi i membri sono vec4 per semplificare l'allineamento di memoria std140
  // (position.w viene ignorata, color.w memorizza l'intensità della luce)
  struct PointLight {
    glm::vec4 position{}; // ignore w
    glm::vec4 color{};    // w is intensity
  };

  // Uniform Buffer Object (UBO) globale: permette di passare dati arbitrari in sola lettura agli shader
  // superando i limiti di dimensione delle push constants (128 byte garantiti vs almeno 16KB per gli UBO).
  struct GlobalUbo {
    // Matrici projection e view separate: permette agli shader (come nei billboard) di estrarre i vettori Up e Right della camera
    glm::mat4 projection{ 1.f };
    glm::mat4 view{ 1.f };
    glm::mat4 inverseView{ 1.f }; // Matrice di vista inversa per ricavare la posizione della camera nello spazio mondo
    // Colore della luce ambientale (RGB) con intensità memorizzata nella componente w (0.02)
    glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };
    PointLight pointLights[MAX_LIGHTS]; // Array di point light fisse inviate agli shader
    int numLights;                      // Numero effettivo di point light attive nella scena corrente
  };

  // Incapsula tutte le informazioni rilevanti per il frame corrente (indice, delta time, command buffer e camera).
  // Raggruppare questi dati in un unico oggetto evita di dover aggiornare la firma di tutti i metodi
  // dei sottosistemi di rendering ogni volta che viene aggiunto un nuovo parametro per-frame.
  struct FrameInfo {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    LveCamera& camera;
    VkDescriptorSet globalDescriptorSet;    // Descriptor set globale per il frame corrente (contiene l'UBO)
    LveDescriptorPool& frameDescriptorPool; // Pool dinamico per allocare descrittori per-oggetto resettato a inizio frame
    LveGameObject::Map& gameObjects;        // Riferimento a tutti i game object attivi nella scena
  };

} // namespace lve