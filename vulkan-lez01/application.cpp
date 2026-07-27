#include "application.h"
#include "utils.h"

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <fmt/ranges.h>

#define VOLK_IMPLEMENTATION
#include <volk.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

Application::Application() { initialized = initialize(); }
Application::~Application() { shutdown(); }

void Application::showError(const std::string& errorMessasge) const
{
    spdlog::error(errorMessasge);
    // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", errorMessasge.c_str(), window);
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        spdlog::error("Validation Layer: {}", pCallbackData->pMessage);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        spdlog::warn("Validation Layer: {}", pCallbackData->pMessage);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        spdlog::info("Validation Layer: {}", pCallbackData->pMessage);
    }
    else {
        spdlog::debug("Validation Layer: {}", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

// Inizializza SDL, crea la finestra, e avvia l'inizializzazione di Vulkan
// L'inizializzazione del modulo video di SDL3 è propedeutica alla creazione di una finestra
// compatibile con Vulkan. Il flag SDL_WINDOW_VULKAN è essenziale per poter successivamente creare
// la "Vulkan surface", un requisito fondamentale per generare la swapchain e disegnare a schermo.
bool Application::initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        showError("Unable to initialize SDL3");
        return false;
    }

    window = SDL_CreateWindow("Vulkan Learning",
        width, height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

    if (window == nullptr)
    {
        std::string errMsg = std::string("Error creating window: ") + SDL_GetError();
        showError(errMsg);
        return false;
    }

    SDL_SetWindowMinimumSize(window, 200, 200);

    if (!initializeVulkan())
    {
        showError("Error initializing vulkan");
        return false;
    }

    return true;
}

// Distrugge tutte le risorse di Vulkan in ordine inverso di creazione per evitare memory leaks
void Application::shutdown()
{
    // wait in case resources are in use
    if (device)
        vkDeviceWaitIdle(device);

    // frame / sync object cleanup
    if (timelineSemaphore)
    {
        vkDestroySemaphore(device, timelineSemaphore, nullptr);
    }
    for (auto& res : frameResources)
    {
        if (res.imageAcquiredSemaphore)
            vkDestroySemaphore(device, res.imageAcquiredSemaphore, nullptr);
        if (res.commandPool)
            vkDestroyCommandPool(device, res.commandPool, nullptr); // destroys buffers implicitly
    }

    // pipeline cleanup
    if (pipelineLayout)
    {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    }
    if (pipeline)
    {
        vkDestroyPipeline(device, pipeline, nullptr);
    }

    // cleanup shaders
    if (vertShader)
    {
        vkDestroyShaderModule(device, vertShader, nullptr);
    }
    if (fragShader)
    {
        vkDestroyShaderModule(device, fragShader, nullptr);
    }

    // cleanup swapchain
    destroySwapchain();

    // VMA
    if (vmaAllocator)
    {
        vmaDestroyAllocator(vmaAllocator);
    }

    // cleanup Vulkan
    if (surface)
    {
        vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
    }
    if (device)
    {
        vkDestroyDevice(device, nullptr);
    }
    if (debugMessenger)
    {
        vkDestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
    }

    if (vulkanInstance)
    {
        vkDestroyInstance(vulkanInstance, nullptr);
    }
    volkFinalize();

    // cleanup SDL
    if (window)
    {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();

    spdlog::debug("Cleanup completed correctly");
}

// Loop principale: processa eventi finestra (come il resize o la chiusura) e chiama il render()
void Application::run()
{
    if (initialized == false)
    {
        spdlog::info("Application not correctly initialized");
        return;
    }

    running = true;
    while (running)
    {
        SDL_Event event{ 0 };
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
                break;
            }
            else if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                width = event.window.data1;
                height = event.window.data2;
                break;
            }
        }

        render();
    }
}

// Sequenza passo-passo per il bootstrap completo di Vulkan
bool Application::initializeVulkan()
{
    if (!createVulkanInstance())
    {
        showError("Couldn't create a vulkan instance");
        return false;
    }

    if (!createSurface())
    {
        showError("Couldn't create window surface");
        return false;
    }

    if (physicalDevice = findPhysicalDevice(); !physicalDevice)
    {
        showError("Unable to find an appropriate physical device");
        return false;
    }

    if (!findGraphicsQueue())
    {
        showError("Unable to find a compatible graphics queue");
        return false;
    }

    if (!createDevice(physicalDevice))
    {
        showError("Couldn't create the logical GPU device");
        return false;
    }

    if (!initializeVMA())
    {
        showError("Unable to create Vulkan Memory Allocator");
        return false;
    }

    if (!createSwapchain(width, height))
    {
        showError("Unable to create swapchain");
        return false;
    }

    if (!createShaders())
    {
        showError("Error creating shader modules");
        return false;
    }

    if (pipeline = createGraphicsPipeline(); !pipeline)
    {
        showError("Unable to initialize the graphics pipeline");
        return false;
    }

    if (!createSyncResources())
    {
        showError("Couldn't create the sync related resources");
        return false;
    }

    if (!createCommandBuffers())
    {
        showError("Couldn't create command buffer objects");
        return false;
    }

    return true;
}

// Inizializza Volk, crea l'Istanza Vulkan e abilita i Validation Layers e le estensioni SDL
// Si utilizza Volk per caricare dinamicamente i puntatori alle funzioni dell'API Vulkan.
// VkApplicationInfo introduce il campo sType, che in Vulkan è onnipresente ed è usato internamente
// per il casting generico e l'estendibilità tramite pNext. Vengono inoltre abilitati i Validation Layers
// (VK_LAYER_KHRONOS_validation) e la callback di debug (VK_EXT_debug_utils), strumenti indispensabili 
// per diagnosticare errori o problemi di performance durante l'intero ciclo di sviluppo.
bool Application::createVulkanInstance()
{
    // Initialize Volk and load Vk function pointers
    if (volkInitialize() != VK_SUCCESS)
    {
        showError("Error initializing Volk");
        return false;
    }

    // Create the vulkan application instance
    VkApplicationInfo appInfo
    {
        // La struttura VkApplicationInfo definisce i metadati dell'applicazione.
        // Il campo sType è obbligatorio per quasi tutte le strutture Vulkan e serve per il casting interno.
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "My First Triangle",
        .apiVersion = VulkanVersion,
    };

    uint32_t instExtCount = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&instExtCount);

    spdlog::debug("sdlExtensions (size {}): [{}]",
        instExtCount,
        fmt::join(sdlExtensions, sdlExtensions + instExtCount, ", "));

    // Copia le estensioni SDL in un vector (dobbiamo aggiungerne una)
    std::vector<const char*> extensions;
    extensions.reserve(instExtCount + 1);
    for (uint32_t i = 0; i < instExtCount; i++)
        extensions.push_back(sdlExtensions[i]);

    // Abilita l'estensione per i messaggi di debug, validation layers e marcatori GPU
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Su macOS: abilita l'estensione di portability
#if defined(__APPLE__)
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    spdlog::debug("extensions (size {}): [{}]",
        extensions.size(),
        fmt::join(extensions, ", "));

    // we'll also need to enable the validation layer for error checking and reporting
    std::vector<const char*> requestedLayers
    {
        "VK_LAYER_KHRONOS_validation"
    };

    VkDebugUtilsMessengerCreateInfoEXT debugInfo
    {
        // Configura il filtro di severità e tipologia dei messaggi per la callback di debug.
        // Consente di isolare errori, warning o messaggi di performance provenienti dal Validation Layer.
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback
    };

    VkInstanceCreateInfo instCreateInfo
    {
        // Il pattern pNext permette di formare una lista concatenata di strutture. In questo caso viene usato per passare la callback di debug.
        // Questa struttura principale raccoglie le estensioni e i validation layers richiesti, 
        // insieme alle informazioni dell'app (appInfo), per istanziare l'oggetto VkInstance.
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debugInfo,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requestedLayers.size()),
        .ppEnabledLayerNames = requestedLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    // Su macOS: abilita il flag di enumerazione portability
#if defined(__APPLE__)
    instCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    if (vkCreateInstance(&instCreateInfo, nullptr, &vulkanInstance) != VK_SUCCESS)
    {
        return false;
    }

    // chiedo a volk gli indirizzi di tutte le funzioni necessarie per interagire con vulkan
    volkLoadInstance(vulkanInstance);

    if (vkCreateDebugUtilsMessengerEXT(vulkanInstance, &debugInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        showError("Failed to set up debug messenger!");
        return false;
    }

    return true;
}

// Collega la finestra fisica di SDL alla superficie di presentazione Vulkan
bool Application::createSurface()
{
    if (!SDL_Vulkan_CreateSurface(window, vulkanInstance, nullptr, &surface))
    {
        return false;
    }
    return true;
}

// Esamina le GPU disponibili e ne seleziona una idonea (preferendo GPU Dedicate)
// Per semplificare le fasi iniziali e arrivare rapidamente a renderizzare a schermo, 
// viene saltata la complessa logica di assegnazione di punteggi all'hardware, limitandosi
// a cercare e selezionare la prima GPU dedicata (Discrete GPU) disponibile nel sistema.
VkPhysicalDevice Application::findPhysicalDevice()
{
    // enumerate all physical devices
    // Tipico pattern Vulkan per l'enumerazione: la prima chiamata con nullptr serve solo a ottenere 
    // il conteggio degli elementi. Dopodiché si alloca lo spazio necessario e la seconda chiamata popola i dati.
    uint32_t physDeviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanInstance, &physDeviceCount, nullptr); // la prima chiamata è per ottenere il numero di dispositivi gpu disponibili
    std::vector<VkPhysicalDevice> physicalDevices(physDeviceCount);
    vkEnumeratePhysicalDevices(vulkanInstance, &physDeviceCount, physicalDevices.data()); // la seconda chiamata invece serve proprio per ottenerli

    VkPhysicalDevice physicalDevice = nullptr;
    if (physDeviceCount)
    {
        // if you have issues, you can always just hardcode a GPU index while learning
        physicalDevice = physicalDevices[0]; // default to first GPU

        // look through list and see if a dGPU exists
        for (auto& pDev : physicalDevices)
        {
            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(pDev, &props);

            // cerco una GPU dedicata (NON una GPU integrata)
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                physicalDevice = pDev;
                break;
            }
        }
    }

    // ensure the desired swapchain format is supported
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());

    bool formatSupported = false;
    for (const VkSurfaceFormatKHR& surfFormat : surfaceFormats)
    {
        if (surfFormat.format == swapchainFormat)
        {
            formatSupported = true;
            break;
        }
    }
    if (!formatSupported)
    {
        showError("Requested swapchain format is not supported by the surface");
        return nullptr;
    }

    return physicalDevice;
}

// Identifica l'indice della Queue Family in grado di gestire sia grafica che presentazioni
// Le Queue Families fungono da code di sottomissione ("tubi") verso specifici motori hardware della GPU.
// Per il rendering a schermo è essenziale individuare una coda che supporti sia i comandi grafici 
// (VK_QUEUE_GRAPHICS_BIT) sia la presentazione diretta sulla surface (Surface Support).
bool Application::findGraphicsQueue()
{
    // eventually we'll have more complex queue lookup for presentation, etc
    // grab all of the queue families
    uint32_t queueFamCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamCount, nullptr);
    std::vector<VkQueueFamilyProperties2> queueFamProps(queueFamCount, { VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 });
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamCount, queueFamProps.data());

    for (int currentFamIdx = 0; currentFamIdx < queueFamProps.size(); currentFamIdx++)
    {
        // ensure it has presentation support
        VkBool32 hasPresentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, currentFamIdx, surface, &hasPresentSupport);

        const auto& props = queueFamProps[currentFamIdx];
        // ensure this is a GRAPHICS queue with presentation support
        if (props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && hasPresentSupport)
        {
            gfxQueueFamIdx = currentFamIdx;
            return true;
        }
    }
    return false;
}


// Crea il dispositivo logico (Device), impostando le Code Grafiche, le Feature richieste (es. Dynamic Rendering) e le Estensioni
// Il Device logico rappresenta una sessione d'uso specifica della GPU fisica sottostante.
// Vengono abilitate unicamente le feature moderne di Vulkan necessarie al progetto (concatenate via pNext):
// - dynamicRendering: elimina la necessità di configurare i prolissi RenderPass/Subpass legacy.
// - synchronization2 e timelineSemaphore: offrono una gestione moderna della sincronizzazione e dei frame-in-flight.
// Abilitare indiscriminatamente feature non utilizzate può impattare negativamente le prestazioni del driver.
bool Application::createDevice(VkPhysicalDevice physicalDevice)
{
    // query suppoted features
    VkPhysicalDeviceVulkan14Features supportedFeatures14{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, .pNext = nullptr };
    VkPhysicalDeviceVulkan13Features supportedFeatures13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &supportedFeatures14 };
    VkPhysicalDeviceVulkan12Features supportedFeatures12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &supportedFeatures13 };
    VkPhysicalDeviceFeatures2 supportedFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &supportedFeatures12 };
    vkGetPhysicalDeviceFeatures2(physicalDevice, &supportedFeatures);

    // check if what we need is supported
    if (!supportedFeatures13.dynamicRendering ||
        !supportedFeatures13.synchronization2 ||
        !supportedFeatures12.timelineSemaphore)
    {
        showError("Physical device doesn't meet the feature requirements");
        return false;
    }

    // è importante abilitare SOLO le feature che si utilizzano
    // produce a separate features struct chain for device creation
    VkPhysicalDeviceVulkan14Features features14
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext = nullptr,
    };
    VkPhysicalDeviceVulkan13Features features13
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &features14,
        .synchronization2 = VK_TRUE, // modernized synchronization
        .dynamicRendering = VK_TRUE, // no more pesky renderpass/subpasses
    };
    VkPhysicalDeviceVulkan12Features features12
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &features13,
        .timelineSemaphore = VK_TRUE // streamlined multi-frame-in-flight handling
    };
    VkPhysicalDeviceFeatures2 features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features12
    };

    // request the queues we'll be using
    std::vector<float> queuePriority{ 1.0f };
    std::vector<uint32_t> queueFamiles{ gfxQueueFamIdx };

    VkDeviceQueueCreateInfo gfxQueueInfo
    {
        // Definisce il numero e la priorità delle code (Queue) appartenenti 
        // a uno specifico indice (queueFamilyIndex) da allocare sul Device Logico.
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = gfxQueueFamIdx,
        .queueCount = 1,
        .pQueuePriorities = queuePriority.data()
    };

    // device specific extensions
    std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#if defined(__APPLE__)
    deviceExtensions.push_back("VK_KHR_portability_subset");
#endif

    VkDeviceCreateInfo devCreateInfo
    {
        // Raggruppa tutte le richieste per creare il Device Logico: Queue richieste, estensioni di livello device (come la swapchain)
        // e le feature attivate (allegate dinamicamente in cascata tramite pNext). Il campo pEnabledFeatures legacy resta a nullptr.
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &gfxQueueInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = nullptr // features struct chain is set in pNext (modern vulkan)
    };

    if (vkCreateDevice(physicalDevice, &devCreateInfo, nullptr, &device) != VK_SUCCESS)
    {
        return false;
    }

    // grab the VkQueue object finally
    vkGetDeviceQueue(device, gfxQueueFamIdx, 0, &gfxQueue);
    if (!gfxQueue)
    {
        showError("Couldn't get the graphics queue");
        return false;
    }
    return true;
}

// Inizializza la libreria Vulkan Memory Allocator, per evitare di dover gestire l'allocazione della memoria GPU a mano
// L'uso di VMA è ormai uno standard per eliminare il vasto boilerplate normalmente richiesto per gestire
// la memoria in Vulkan (enumerazione dei tipi, controlli dei requisiti, ecc.). Viene anche abilitato
// l'accesso alla VRAM tramite Device Address, funzionalità utile per il futuro caricamento di scene GLTF.
bool Application::initializeVMA()
{
    VmaVulkanFunctions vmaFuncInfo{};
    VmaAllocatorCreateInfo vmaAllocInfo
    {
        // Inizializza l'allocatore indicando il Device, l'Istanza e i puntatori alle funzioni Vulkan.
        // Il flag DEVICE_ADDRESS_BIT abilita l'uso dei moderni puntatori per accedere alla VRAM direttamente.
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT, // permette di accedere direttamente alle shaders in VRAM con un puntatore
        .physicalDevice = physicalDevice,
        .device = device,
        .pVulkanFunctions = &vmaFuncInfo,
        .instance = vulkanInstance,
        .vulkanApiVersion = VulkanVersion
    };

    // vma can import directly from volk
    vmaImportVulkanFunctionsFromVolk(&vmaAllocInfo, &vmaFuncInfo);

    if (vmaCreateAllocator(&vmaAllocInfo, &vmaAllocator) != VK_SUCCESS)
    {
        return false;
    }
    return true;
}

// Crea la catena di scambio (Swapchain) per presentare i frame a schermo ed alloca i buffer di Depth
// La swapchain è costituita da un pool di immagini fornite in "prestito" dal sistema operativo, che
// vengono acquisite, manipolate dal renderer e poi restituite per la presentazione a schermo.
// Si utilizza la modalità VK_PRESENT_MODE_FIFO_KHR, che assicura compatibilità universale e funge da V-Sync.
bool Application::createSwapchain(uint32_t width, uint32_t height)
{
    // track swapchain size separate from window size
    swapchainWidth = width;
    swapchainHeight = height;

    // ensure we request an appropriate number of images
    VkSurfaceCapabilitiesKHR surfaceCaps{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps) != VK_SUCCESS)
    {
        showError("Couldn't get the surface capabilities");
        return false;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo
    {
        // Delinea come sarà costruita la swapchain, incluso il numero minimo di immagini,
        // la risoluzione (extent), il formato colore e il modo in cui presentare le immagini a schermo.
        // presentMode impostato su FIFO si assicura che il VSync sia abilitato.
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surfaceCaps.minImageCount,
        .imageFormat = swapchainFormat,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent{.width = swapchainWidth, .height = swapchainHeight },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,

        // Impone al compositor di considerare la finestra interamente opaca (senza fusione alpha col desktop).
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,

        .presentMode = VK_PRESENT_MODE_FIFO_KHR
    };

    if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS)
    {
        showError("Error creating swapchain");
        return false;
    }

    // grab the swapchain images
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
    swapchainImageViews.resize(imageCount);

    // create the swapchain image views
    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        VkImageViewCreateInfo imgViewInfo
        {
            // Crea una vista (ImageView) su una specifica immagine grezza della Swapchain.
            // Indica a Vulkan di trattarla come una texture 2D standard con 1 livello di Mipmap.
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchainFormat,
            .subresourceRange
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        if (vkCreateImageView(device, &imgViewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
        {
            showError("Error creating swapchain image view");
            return false;
        }
    }

    // semaphores used to signal render completion
    renderCompleteSemaphores.resize(swapchainImages.size());
    for (VkSemaphore& semaphore : renderCompleteSemaphores)
    {
        VkSemaphoreCreateInfo semaphoreInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
        {
            showError("Error creating the render-complete semaphore");
            return false;
        }
    }

    // create depth image
    VkImageCreateInfo depthCreateInfo
    {
        // Descrive la geometria e le proprietà della Depth Image. Il flag DEPTH_STENCIL_ATTACHMENT_BIT 
        // indica che lo scopo di questa immagine è fungere da Z-Buffer. Il tiling ottimale ottimizza 
        // le allocazioni affinché l'accesso in memoria sia veloce.
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent{.width = swapchainWidth, .height = swapchainHeight, .depth = 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VmaAllocationCreateInfo allocInfo
    {
        // Struttura di VMA che richiede un'allocazione di memoria dedicata per il depth buffer.
        // VMA_MEMORY_USAGE_AUTO demanda la decisione su dove posizionare il buffer direttamente alla libreria.
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    if (vmaCreateImage(vmaAllocator, &depthCreateInfo, &allocInfo, &depthImage, &depthImageAllocation, nullptr) != VK_SUCCESS)
    {
        showError("Error allocating depth image");
        return false;
    }

    VkImageViewCreateInfo depthImgViewInfo
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depthImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depthFormat,
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1}
    };
    if (vkCreateImageView(device, &depthImgViewInfo, nullptr, &depthImageView) != VK_SUCCESS)
    {
        showError("Error creating depth image view");
        return false;
    }

    return true;
}

// Distrugge gli oggetti legati alla Swapchain (da chiamare ad esempio quando la finestra viene ridimensionata)
void Application::destroySwapchain()
{
    for (VkImageView swapchainImgView : swapchainImageViews)
    {
        vkDestroyImageView(device, swapchainImgView, nullptr);
    }
    swapchainImageViews.clear();

    // destroy render-complete ssemaphores
    for (VkSemaphore& semaphore : renderCompleteSemaphores)
    {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    renderCompleteSemaphores.clear();

    if (swapchain)
    {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = nullptr;
    }

    // destroy the depth buffer along with the swapchain
    if (depthImageView)
    {
        vkDestroyImageView(device, depthImageView, nullptr);
        vmaDestroyImage(vmaAllocator, depthImage, depthImageAllocation);
        depthImageView = nullptr;
    }
}

// Legge un file shader dal disco (in GLSL) e lo compila al volo in formato SPIR-V usando shaderc
// Vulkan consuma nativamente shader in formato bytecode SPIR-V. L'uso di shaderc permette di compilare
// i sorgenti GLSL testuali direttamente a runtime, offrendo maggiore flessibilità rispetto alle build offline.
// Nota: è necessario che l'SDK disponga dei debug symbols per collegare correttamente la libreria del compilatore.
VkShaderModule Application::createShaderModule(const std::string& fileName, shaderc_shader_kind kind) const
{
    // read shader file from disk
    const std::string shaderPath = std::string("shaders/") + fileName;
    const std::string src = readTextFile(shaderPath);
    if (src.empty())
    {
        showError(std::string("Specified shader file doesn't exist: ") + shaderPath);
        return nullptr;
    }

    // compile the shader to SPIR-V
    spdlog::info("Compiling shader: {}", shaderPath);
    shaderc::Compiler compiler;
    shaderc::CompileOptions opts;
    opts.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
    opts.SetTargetSpirv(shaderc_spirv_version_1_6);
    opts.SetOptimizationLevel(shaderc_optimization_level_performance);
    auto result = compiler.CompileGlslToSpv(src, kind, fileName.c_str(), opts);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        showError("Error compiling shader" + result.GetErrorMessage());
        return nullptr;
    }
    std::vector<uint32_t> spv = { result.cbegin(), result.cend() };

    // pass spir-v to vulkan and create shader-module
    VkShaderModuleCreateInfo moduleCreateInfo
    {
        // VkShaderModuleCreateInfo incapsula il bytecode SPIR-V caricato in memoria per creare il modulo shader.
        // pCode si aspetta un array di interi a 32-bit (uint32_t) indicante il codice compilato.
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spv.size() * sizeof(uint32_t),
        .pCode = spv.data()
    };
    VkShaderModule shaderModule = nullptr;
    if (vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        showError("Error creating shader module");
        return nullptr;
    }
    return shaderModule;
}

// Avvia la creazione dei Vertex e Fragment shader essenziali per la pipeline
bool Application::createShaders()
{
    // create the shader modules that we'll need for the graphics pipeline
    if (vertShader = createShaderModule("shader.vert", shaderc_vertex_shader); !vertShader)
    {
        return false;
    }
    if (fragShader = createShaderModule("shader.frag", shaderc_fragment_shader); !fragShader)
    {
        return false;
    }
    return true;
}

// Imposta la Pipeline Grafica: Vertex Input, Rasterizer, Viewport, Depth/Stencil e la fonde con gli shader compilati
// La pipeline raggruppa la maggior parte dello stato della GPU. Sfruttando il Dynamic Rendering tramite
// VkPipelineRenderingCreateInfo, si bypassa totalmente la prolissa definizione di un RenderPass tradizionale.
// Viewport e Scissor sono configurati come Dynamic State, consentendo di adattarli fluidamente al ridimensionamento
// della finestra senza richiedere la ricreazione dell'intera pipeline.
VkPipeline Application::createGraphicsPipeline()
{
    // need to define a pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo
    {
        // Il PipelineLayout agisce come la "firma" di una funzione per gli shader.
        // Attualmente è configurato vuoto (zero descriptor sets e zero push constants) 
        // in quanto il nostro triangolo minimale non riceve dati extra dall'esterno.
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0
    };
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        showError("Unable to create the pipeline layout");
        return nullptr;
    }

    // configure the shader stages struct
    const char* entryPoint = "main";
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages
    {
        {
            // Associa il Vertex Shader precedentemente compilato allo stadio di elaborazione dei vertici.
            // Indica inoltre il nome della funzione punto di ingresso (pName = "main").
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShader,
            .pName = entryPoint
        },
        {
            // Associa il Fragment Shader compilato allo stadio di frammentazione (pixel rendering).
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShader,
            .pName = entryPoint
        }
    };

    // vertex pulling, don't define vertex input details
    VkPipelineVertexInputStateCreateInfo vertInputInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };

    // input assembly, we'll be drawing triangle lists
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo
    {
        // Specifica in quale modo i vertici processati debbano essere assemblati in primitive geometriche.
        // TRIANGLE_LIST raggruppa i vertici in gruppi da 3 senza condivisione di lati.
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    // depth/stencil configuration
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo
    {
        // Abilita la comparazione e scrittura nel Depth Buffer (Z-Buffer).
        // VK_COMPARE_OP_LESS assicura che solo i frammenti più vicini alla telecamera (profondità minore)
        // passino il test, nascondendo automaticamente la geometria che si trova dietro.
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .stencilTestEnable = VK_FALSE
    };

    // dynamic rendering allows to set this up...dynamically
    // we still need this struct though
    VkPipelineViewportStateCreateInfo viewportInfo
    {
        // Anche se Viewport e Scissor sono stati dichiarati come stati dinamici (Dynamic State),
        // è comunque obbligatorio notificare a Vulkan il numero (count) di viewport che si intendono utilizzare.
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr
    };

    // rasterizer settings
    VkPipelineRasterizationStateCreateInfo rasterInfo
    {
        // Definisce il Rasterizzatore: polygonMode = FILL fa riempire l'intero triangolo.
        // frontFace = COUNTER_CLOCKWISE significa che l'ordine antiorario dei vertici indica una faccia frontale.
        // cullMode = BACK abilita il culling eliminando (non disegnando) le facce posteriori.
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    // No multisampling
    VkPipelineMultisampleStateCreateInfo multiSampleInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    // Alpha-blending (disabled for now), still need
    // attachment info and write mask
    VkPipelineColorBlendAttachmentState attachState
    {
        // Specifica per un dato color attachment (la Swapchain) se debbano avvenire operazioni di alpha blending.
        // Per ora disattivato (blendEnable = VK_FALSE). colorWriteMask istruisce su quali canali scrivere (RGBA).
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };
    VkPipelineColorBlendStateCreateInfo blendInfo
    {
        // Aggrega le impostazioni globali del color blending (e i relativi attachment states)
        // per finalizzare lo stadio di blending della grafica.
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachState
    };

    // enable dynamic state
    std::vector<VkDynamicState> dynamicState
    {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicStateInfo
    {
        // Segnala esplicitamente alla Pipeline quali stati potranno variare al volo durante il render.
        // Viewport e Scissor dinamici permettono di aggiornare le dimensioni della finestra al resize senza pesanti ricreazioni.
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicState.size()),
        .pDynamicStates = dynamicState.data()
    };

    // structure required for dynamic rendering
    VkPipelineRenderingCreateInfo renderInfo
    {
        // È il nucleo del Dynamic Rendering: bypassa completamente la complessa architettura dei RenderPass.
        // Fornisce direttamente i formati colore e profondità previsti (agganciata tramite pNext alla pipeline principale).
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchainFormat,
        .depthAttachmentFormat = depthFormat
    };

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo
    {
        // Raccoglie e compila ogni singolo stato configurato sinora (Shader, Vertex, InputAssembly, Rasterizer, ecc.).
        // Indica esattamente alla GPU come e in quale forma processare i futuri comandi di draw.
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderInfo,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState = &viewportInfo,
        .pRasterizationState = &rasterInfo,
        .pMultisampleState = &multiSampleInfo,
        .pDepthStencilState = &depthStencilInfo,
        .pColorBlendState = &blendInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = pipelineLayout,
        .renderPass = VK_NULL_HANDLE,
    };
    if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        showError("Error creating the pipeline");
        return nullptr;
    }
    return pipeline;
}

// Inizializza i semafori per la sincronizzazione precisa tra CPU e GPU (Timeline Semaphore e Binary Semaphores)
bool Application::createSyncResources()
{
    VkSemaphoreTypeCreateInfo semaphoreTypeInfo
    {
        // Utilizzata per creare un Timeline Semaphore, che permette una sincronizzazione basata
        // su contatori interi crescenti anziché sul classico segnale binario. È un'estensione molto utile
        // in caso di multi-threading e sincronizzazioni avanzate dei frame-in-flight.
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = MaxFramesInFlight
    };
    VkSemaphoreCreateInfo semaphoreInfo
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &semaphoreTypeInfo
    };
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &timelineSemaphore) != VK_SUCCESS)
    {
        showError("Unable to create the timeline semaphore");
        return false;
    }

    // per-frame image-acquire semaphores
    for (FrameResources& res : frameResources)
    {
        // create the binary semaphores
        VkSemaphoreCreateInfo semaphoreInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &res.imageAcquiredSemaphore) != VK_SUCCESS)
        {
            showError("Error creating the per-frame image-acquire semaphore");
            return false;
        }
    }

    return true;
}

// Crea i Command Pools ed alloca i Command Buffers usati per registrare e inviare istruzioni grafiche per ogni frame
// Viene assegnato un Command Pool distinto per ogni frame "in-flight". Questa strategia ottimizza le prestazioni
// permettendo di resettare rapidamente l'intero pool in blocco, un'operazione molto più efficiente del reset
// individuale dei singoli Command Buffer.
bool Application::createCommandBuffers()
{
    for (FrameResources& res : frameResources)
    {
        // we'll give each frame its own pool, faster cmd buffer resets this way
        VkCommandPoolCreateInfo poolInfo
        {
            // Il Command Pool alloca e ricicla memoria per i Command Buffers. 
            // In questo caso il pool è ottimizzato per code di tipo grafico (gfxQueueFamIdx).
            // L'uso di un pool per frame consente un reset rapidissimo dei comandi al frame successivo.
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = gfxQueueFamIdx
        };
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &res.commandPool) != VK_SUCCESS)
        {
            showError("Unable to create command buffer pool");
            return false;
        }

        // create the command buffer for this frame
        VkCommandBufferAllocateInfo cmdAllocInfo
        {
            // Richiede l'allocazione di un singolo Buffer "Primario". I primary command buffer
            // possono essere inviati direttamente alla Queue per l'esecuzione.
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = res.commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        if (vkAllocateCommandBuffers(device, &cmdAllocInfo, &res.commandBuffer) != VK_SUCCESS)
        {
            showError("Unable to allocate command buffer");
            return false;
        }
    }
    return true;
}

// Il vero cuore dell'app: sincronizza i frame, acquisisce l'immagine, registra i comandi, esegue il rendering dinamico e presenta a schermo
// Il flusso logico principale per ogni frame consiste in:
// 1. Acquisizione dell'immagine successiva dalla swapchain.
// 2. Inserimento di Pipeline Barriers (sfruttando Synchronization2) per transizionare l'immagine al layout COLOR_ATTACHMENT_OPTIMAL.
// 3. Avvio del Dynamic Rendering, configurazione dei dynamic state (viewport/scissor) e submission della drawcall.
// 4. Nuova transizione dell'immagine al layout PRESENT_SRC_KHR.
// 5. Submit definitivo dei Command Buffer alla Graphics Queue e richiesta di presentazione a schermo.
void Application::render()
{
    // first check if our swapchain is still valid
    if (requireSwapchainRecreate)
    {
        vkDeviceWaitIdle(device);
        destroySwapchain();
        createSwapchain(width, height);
        requireSwapchainRecreate = false;
    }

    const uint32_t frameResIndex = frameIndex++ % MaxFramesInFlight;
    const uint64_t signalValue = nextSignalValue++;
    const uint64_t waitValue = signalValue - MaxFramesInFlight;

    VkSemaphoreWaitInfo waitInfo
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &timelineSemaphore,
        .pValues = &waitValue
    };
    vkWaitSemaphores(device, &waitInfo, UINT64_MAX);

    // now its safe to start recording commands
    FrameResources& res = frameResources[frameResIndex];
    vkResetCommandPool(device, res.commandPool, 0);

    // get the resources for this frame
    VkSemaphore imageAcquireSemaphore = frameResources[frameResIndex].imageAcquiredSemaphore;

    uint32_t imageIndex = 0;
    VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquireSemaphore, VK_NULL_HANDLE, &imageIndex);

    // Gestisce attivamente i resize o la minimizzazione della finestra. 
    // Se la swapchain risulta invalidata (out-of-date), viene segnalata per essere ricreata prima del prossimo ciclo.
    // handle resize and out-of-date images, may need swapchain recreate
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // VK_ERROR_OUT_OF_DATE_KHR: La Swapchain è rotta e completamente inutilizzabile.
        // Non puoi neanche finire di disegnare il fotogramma corrente; devi interrompere il
        // rendering immediatamente e ricreare la Swapchain.

        // Cosa significa: La superficie della finestra del sistema operativo è cambiata a tal punto 
        // che le immagini attualmente allocate nella Swapchain non sono più compatibili con la superficie
        // di presentazione.
        //
        // Quando si verifica: Succede quando si ridimensiona la finestra, la si minimizza a icona 
        // (portando temporaneamente le dimensioni a 0x0) o la si sposta su un altro monitor con scala DPI
        // o frequenza di aggiornamento differenti. 

        requireSwapchainRecreate = true;
        return;
    }
    else if (acquireResult == VK_SUBOPTIMAL_KHR)
    {
        // VK_SUBOPTIMAL_KHR: La Swapchain funziona ancora, ma non è più perfetta. 
        // Puoi terminare di disegnare e mostrare il fotogramma attuale, ma devi impostare un 
        //flag per ricrearla al fotogramma successivo.

        // Cosa significa: La Swapchain è ancora in grado di inviare l'immagine allo schermo, 
        // ma le sue proprietà non corrispondono esattamente alla dimensione della finestra 
        // (ad esempio la Swapchain è 1280 x 720 mentre la finestra è appena diventata 
        // 1282 x 720). Il sistema operativo applicherà uno scaling temporaneo per adattare 
        // l'immagine, il che può causare un leggero calo di prestazioni o sfocatura visiva.
        //
        // Quando si verifica: Accade tipicamente mentre l'utente sta trascinando attivamente i 
        // bordi della finestra con il mouse per ridimensionarla.

        // can render this frame, recreate next time around
        requireSwapchainRecreate = true;
    }

    // begin recording commands
    VkCommandBufferBeginInfo cmdBeginInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(res.commandBuffer, &cmdBeginInfo);

    // transition the color and depth images (Synchronization2)
    // Prima di iniziare il rendering dinamico, dobbiamo sincronizzare l'accesso alla memoria 
    // e cambiare la disposizione dei dati (layout) sia dell'immagine della Swapchain (colore) 
    // sia del Depth Buffer (profondità) per portarle nei rispettivi formati ottimali di scrittura.
    std::vector<VkImageMemoryBarrier2> layoutBarriers
    {
        {
            // Barriera di memoria per transizionare l'immagine della Swapchain 
            // da un layout iniziale indefinito al layout ottimale per riceverne i colori in rendering.

            // --- 1. BARRIERA PER L'IMMAGINE COLORE (SWAPCHAIN) ---
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,

            // Stadio e accessi precedenti: non ci interessa preservare o attendere operazioni passate
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,

            // Stadio e accessi futuri: blocca le scritture del colore finché la transizione non è completa
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,

            // Transizione di layout: da indefinito (il contenuto precedente viene scartato) a ottimale per il colore
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,

            .image = swapchainImages[imageIndex],
            .subresourceRange
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // Applica solo al canale colore
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        },
        {
            // --- 2. BARRIERA PER L'IMMAGINE DI PROFONDITÀ (DEPTH BUFFER) ---
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,

            // Stadio di partenza per i test di profondità
            .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,

            // Blocca la scrittura nello Z-Buffer sia nel test di profondità anticipato (Early) che in quello posticipato (Late)
            .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,

            // Transizione di layout: porta l'immagine nel formato memoria ideale per i calcoli dello Z-Buffer
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,

            .image = depthImage,
            .subresourceRange
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, // Applica solo all'aspetto di profondità
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        }
    };
    VkDependencyInfo depInfo
    {
        // Raggruppa le barriere di memoria (sia per l'immagine colore che per il depth) in un'unica 
        // sottomissione per sincronizzare adeguatamente l'accesso alle risorse prima di disegnare.
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = static_cast<uint32_t>(layoutBarriers.size()),
        .pImageMemoryBarriers = layoutBarriers.data()
    };
    vkCmdPipelineBarrier2(res.commandBuffer, &depInfo);

    // setup the attachments (color and depth) and begin rendering (dynamic)
    VkRenderingAttachmentInfo colorAttachInfo
    {
        // Dichiara come verrà usata l'immagine target (color attachment).
        // LOAD_OP_CLEAR pulisce l'immagine a inizio frame usando clearValue.
        // STORE_OP_STORE comunica a Vulkan di salvare il risultato in memoria, fondamentale per poterlo presentare.
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchainImageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // clear the image
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // keep data for presentation
        .clearValue{.color{0.01f, 0.01f, 0.01f, 1}}
    };
    VkRenderingAttachmentInfo depthAttachInfo
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = depthImageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // clear the depth data
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, // don't care after rendering
        .clearValue{.depthStencil{1.0f, 0}}
    };
    VkRenderingInfo renderingInfo
    {
        // Struttura root per il Dynamic Rendering (vkCmdBeginRendering).
        // Definisce l'area di rendering (renderArea) e unisce gli attachment di color e depth necessari.
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea
        {
            .offset{.x = 0, .y = 0},
            .extent{.width = swapchainWidth, .height = swapchainHeight}
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachInfo,
        .pDepthAttachment = &depthAttachInfo
    };

    // begin dynamic rendering
    vkCmdBeginRendering(res.commandBuffer, &renderingInfo);
    {
        // set the viewpot and scissor state
        VkViewport viewport
        {
            .x = 0, .y = 0,
            .width = static_cast<float>(swapchainWidth),
            .height = static_cast<float>(swapchainHeight)
        };
        vkCmdSetViewport(res.commandBuffer, 0, 1, &viewport);

        VkRect2D scissor
        {
            .offset{.x = 0, .y = 0 },
            .extent{.width = swapchainWidth, .height = swapchainHeight}
        };
        vkCmdSetScissor(res.commandBuffer, 0, 1, &scissor);

        // draw our triangle
        vkCmdBindPipeline(res.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdDraw(res.commandBuffer, 3, 1, 0, 0);
    }
    // end dynamic rendering
    vkCmdEndRendering(res.commandBuffer);

    // transition the image from color attachment to presentation so we can show it
    VkImageMemoryBarrier2 presentLayoutBarrier
    {
        // Ulteriore barriera che prepara l'immagine finale per essere presa in carico
        // dal presentation engine, cambiandone il layout in PRESENT_SRC_KHR.
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE, // nothing is waiting, but the cache is flushed and layout is transition
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = swapchainImages[imageIndex],
        .subresourceRange
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    VkDependencyInfo presentDepInfo
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &presentLayoutBarrier
    };
    vkCmdPipelineBarrier2(res.commandBuffer, &presentDepInfo);

    vkEndCommandBuffer(res.commandBuffer);

    // ensure swapchain image is actually vailable to start color output
    VkSemaphoreSubmitInfo imageAcquireWaitInfo
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = imageAcquireSemaphore,
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT // wait before drawing to image
    };
    // signal that the image can be presented
    std::vector<VkSemaphoreSubmitInfo> semaphoreSignals
    {
        {
            // Indica un semaforo binario (non-timeline) da segnalare al termine, informando
            // il presentation engine che può acquisire in modo sicuro l'immagine per mostrarla a schermo.
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = renderCompleteSemaphores[imageIndex],
            .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
        },
        {
            // Indica quale semaforo della timeline incrementare al termine dei lavori (sincronizza i frame-in-flight sulla CPU/GPU).
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = timelineSemaphore,
            .value = signalValue,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
        }
    };
    VkCommandBufferSubmitInfo cmdSubmitInfo
    {
        // Struttura usata per inviare il nostro singolo Command Buffer registrato alla Queue.
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = res.commandBuffer,
    };
    VkSubmitInfo2 submitInfo
    {
        // Consolida l'intera operazione di submit: quali buffer eseguire (cmdSubmitInfo),
        // quali semafori attendere in partenza (semaphoreWaits) e quali semafori segnalare alla fine (semaphoreSignals).
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &imageAcquireWaitInfo, // ensure the image is ready
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdSubmitInfo,
        .signalSemaphoreInfoCount = static_cast<uint32_t>(semaphoreSignals.size()),
        .pSignalSemaphoreInfos = semaphoreSignals.data()
    };
    vkQueueSubmit2(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE);

    // present the image
    VkPresentInfoKHR presentInfo
    {
        // Struttura per la richiesta di presentazione a schermo.
        // Dice all'OS su quale swapchain agire, con quale imageIndex, 
        // e su quali semafori (renderCompleteSemaphores) mettersi in attesa prima di renderla visibile.
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderCompleteSemaphores[imageIndex], // render work completed semaphore
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr
    };

    vkQueuePresentKHR(gfxQueue, &presentInfo);
}
