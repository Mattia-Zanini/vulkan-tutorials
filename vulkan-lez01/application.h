#pragma once

#define VK_NO_PROTOTYPES

#include <vector>
#include <array>
#include <string>

#include "SDL3/SDL.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_main.h"
#include <SDL3/SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <shaderc/shaderc.hpp>

struct SDL_Window;
struct VmaAllocator_T;
typedef struct VmaAllocator_T* VmaAllocator;
struct VmaAllocation_T;
typedef struct VmaAllocation_T* VmaAllocation;

struct FrameResources
{
    VkCommandPool commandPool = nullptr;            // Pool allocatore per i comandi di questo frame
    VkCommandBuffer commandBuffer = nullptr;        // Buffer per registrare le operazioni di rendering
    VkSemaphore imageAcquiredSemaphore = nullptr;   // Semaforo: segnala quando l'immagine è pronta dalla swapchain
};

class Application
{
    constexpr static uint32_t VulkanVersion{ VK_API_VERSION_1_4 };
    constexpr static uint32_t MaxFramesInFlight{ 2 };       // Quanti frame possono essere processati in parallelo
    constexpr static VkFormat swapchainFormat{ VK_FORMAT_B8G8R8A8_SRGB };
    constexpr static VkFormat depthFormat{ VK_FORMAT_D32_SFLOAT };

    // Callback per intercettare i messaggi di debug dei Validation Layers
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    SDL_Window* window = nullptr;
    uint32_t width = 1280;
    uint32_t height = 720;
    bool initialized = false;
    bool running = false;
    uint64_t frameIndex = 0;                        // Tiene traccia del frame corrente per il MaxFramesInFlight
    uint64_t nextSignalValue = MaxFramesInFlight + 1;

    // vulkan core
    VkInstance vulkanInstance = nullptr;            // Punto di ingresso all'API Vulkan
    VkDebugUtilsMessengerEXT debugMessenger = nullptr; // Riceve i log di debug di Vulkan
    VkPhysicalDevice physicalDevice = nullptr;      // La vera e propria scheda video hardware
    VkDevice device = nullptr;                      // Il device logico usato per impartire i comandi
    VkSurfaceKHR surface = nullptr;                 // La superficie su cui si andrà a disegnare (gestita da SDL)
    VmaAllocator vmaAllocator = nullptr;            // Allocatore avanzato per gestire la memoria GPU

    // queue related
    uint32_t gfxQueueFamIdx = UINT32_MAX;           // Indice della famiglia di code grafica
    VkQueue gfxQueue = nullptr;                     // Coda dove vengono inviati i command buffers

    // swapchain related
    VkSwapchainKHR swapchain = nullptr;             // Coda di immagini da presentare sullo schermo
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkSemaphore> renderCompleteSemaphores; // Segnalano il termine del rendering
    bool requireSwapchainRecreate = false;
    uint32_t swapchainWidth = 0;
    uint32_t swapchainHeight = 0;

    VkImage depthImage = nullptr;                   // Immagine per il Depth Buffer (Z-Buffer)
    VkImageView depthImageView = nullptr;
    VmaAllocation depthImageAllocation = nullptr;

    // graphics pipeline related
    VkPipelineLayout pipelineLayout = nullptr;      // Specifica i dati che verranno passati allo shader (es. uniform, push constants)
    VkPipeline pipeline = nullptr;                  // Lo stato e i programmi shader compilati per la pipeline grafica

    // shader resources
    VkShaderModule vertShader = nullptr;
    VkShaderModule fragShader = nullptr;

    // frame and synchronization resources
    VkSemaphore timelineSemaphore = nullptr;        // Semaforo avanzato per sincronizzare i frame su CPU/GPU
    std::array<FrameResources, MaxFramesInFlight> frameResources;

    // Mostra a schermo e logga eventuali errori fatali
    void showError(const std::string& errorMessasge) const;

    // --- Fase di inizializzazione Vulkan ---
    bool initializeVulkan();
    bool createVulkanInstance();
    bool createSurface();
    VkPhysicalDevice findPhysicalDevice();
    bool findGraphicsQueue();
    bool createDevice(VkPhysicalDevice physicalDevice);
    bool initializeVMA();
    bool createSwapchain(uint32_t width, uint32_t height);
    void destroySwapchain();
    VkShaderModule createShaderModule(const std::string& fileName, shaderc_shader_kind kind) const;
    bool createShaders();
    VkPipeline createGraphicsPipeline();
    bool createSyncResources();
    bool createCommandBuffers();

    // --- Fase di rendering (viene chiamata ad ogni iterazione del loop) ---
    void render();

public:
    Application(); // costruttore
    ~Application(); // distruttore

    // Il main loop dell'applicazione
    void run();

private:
    // Inizializza SDL, crea la finestra, e chiama tutta la routine di inizializzazione Vulkan
    bool initialize();
    // Distrugge tutte le risorse allocate (chiamato alla chiusura)
    void shutdown();
};