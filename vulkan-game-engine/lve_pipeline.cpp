#include "lve_pipeline.hpp"
#include "vulkan/vulkan_core.h"

#include <fstream>
#include <iostream>
#include <fmt/format.h>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace lve {
  LvePipeline::LvePipeline(LveDevice& device, const std::string& vertFilePath,
                           const std::string& fragFilePath, const PipelineConfigInfo& configInfo)
      : lveDevice{ device } {
    createGraphicsPipeline(vertFilePath, fragFilePath, configInfo);
  }

  LvePipeline::~LvePipeline() {
    // Rilascio delle risorse Vulkan allocate per i moduli shader e la pipeline
    vkDestroyShaderModule(lveDevice.device(), vertShaderModule, nullptr);
    vkDestroyShaderModule(lveDevice.device(), fragShaderModule, nullptr);
    vkDestroyPipeline(lveDevice.device(), graphicsPipeline, nullptr);
  }

  std::vector<char> LvePipeline::readFile(const std::string& filePath) {
    // std::ios::ate -> vado alla fine del file, utile per ottenenere la dimensione del file
    // std::ios::binary -> leggo il file come stream binario
    std::ifstream file{ filePath, std::ios::ate | std::ios::binary };

    if (!file.is_open()) {
      spdlog::error("failed to open file: {}", filePath);
      throw std::runtime_error(fmt::format("failed to open file: {}", filePath));
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
  }

  void LvePipeline::createGraphicsPipeline(const std::string& vertFilePath,
                                           const std::string& fragFilePath,
                                           const PipelineConfigInfo& configInfo) {
    // Verifica che le risorse obbligatorie esterne siano state fornite prima di creare la pipeline
    assert(configInfo.pipelineLayout != VK_NULL_HANDLE &&
           "Cannot create graphics pipeline:: no pipelineLayout provided in configInfo");
    assert(configInfo.renderPass != VK_NULL_HANDLE &&
           "Cannot create graphics pipeline:: no renderPass provided in configInfo");

    auto vertCode = readFile(vertFilePath);
    auto fragCode = readFile(fragFilePath);

    spdlog::debug("Vertex Shader Code Size: {}", vertCode.size());
    spdlog::debug("Fragment Shader Code Size: {}", fragCode.size());

    // Creazione dei moduli shader a partire dal bytecode SPIR-V compilato
    createShaderModule(vertCode, &vertShaderModule);
    createShaderModule(fragCode, &fragShaderModule);

    // Configurazione degli stadi programmabili (Vertex & Fragment) della pipeline
    VkPipelineShaderStageCreateInfo shaderStages[2];
    // 1. Vertex Shader Stage
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main"; // Nome della funzione entrypoint nello shader
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr; // Per personalizzare costanti nello shader

    // 2. Fragment Shader Stage
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = 0;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;

    // Descrive il formato dei dati dei vertici in ingresso (binding e attributi)
    // Impostato a 0 per ora dato che i vertici sono hardcoded direttamente nel vertex shader
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;

    // Combina Viewport e Scissor.
    // Viene creata come variabile locale inizializzata a zero con le parentesi graffe {} (che
    // garantiscono pNext = nullptr e flags = 0). Non risiede più dentro PipelineConfigInfo per
    // evitare puntatori penzolanti (dangling pointers): se PipelineConfigInfo venisse copiata, i
    // puntatori pViewports e pScissors punterebbero ancora ai membri della vecchia struct
    // deallocata.
    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &configInfo.viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &configInfo.scissor;

    // Assemblaggio della struttura principale per la creazione della pipeline grafica
    // Collega tutti gli stadi programmabili e le configurazioni a funzione fissa definite in
    // configInfo
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2; // Numero di stadi programmabili (Vertex + Fragment)
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
    pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
    pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
    pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
    pipelineInfo.pDynamicState =
        nullptr; // Stati dinamici opzionali (modificabili a runtime senza ricreare la pipeline)

    pipelineInfo.layout = configInfo.pipelineLayout;
    pipelineInfo.renderPass = configInfo.renderPass;
    pipelineInfo.subpass = configInfo.subpass;

    // Parametri per derivare una pipeline da una esistente (ottimizzazione avanzata)
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(
            lveDevice.device(),
            VK_NULL_HANDLE, // Pipeline cache opzionale per velocizzare la compilazione
            1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
      throw std::runtime_error("failed to create graphics pipeline");
    }
  }

  void LvePipeline::createShaderModule(const std::vector<char>& code,
                                       VkShaderModule* shaderModule) {
    VkShaderModuleCreateInfo createinfo{ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                         .codeSize = code.size(),
                                         .pCode = reinterpret_cast<const uint32_t*>(code.data()) };

    if (vkCreateShaderModule(lveDevice.device(), &createinfo, nullptr, shaderModule) !=
        VK_SUCCESS) {
      spdlog::error("failed to create shader module");
      throw std::runtime_error("failed to create shader module");
    }
  }

  PipelineConfigInfo LvePipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height) {
    PipelineConfigInfo configInfo{};

    // ==========================================
    // 1° STADIO: INPUT ASSEMBLER
    // ==========================================
    // Raggruppa i vertici in geometrie primitive. Con TRIANGLE_LIST ogni gruppo di 3 vertici
    // consecutivi forma un triangolo separato. primitiveRestartEnable permette (se true) di
    // spezzare geometrie continue (strip) inserendo un indice speciale nell'index buffer.
    configInfo.inputAssemblyInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // ==========================================
    // VIEWPORT & SCISSOR
    // ==========================================
    // Il Viewport trasforma le coordinate normalizzate di gl_Position ([-1, 1]) nello spazio pixel
    // dell'immagine (width, height). minDepth e maxDepth specificano l'intervallo di profondità per
    // la coordinata Z.
    configInfo.viewport.x = 0.0f;
    configInfo.viewport.y = 0.0f;
    configInfo.viewport.width = static_cast<float>(width);
    configInfo.viewport.height = static_cast<float>(height);
    configInfo.viewport.minDepth = 0.0f;
    configInfo.viewport.maxDepth = 1.0f;

    // Lo Scissor definisce un rettangolo di ritaglio: i pixel al di fuori vengono scartati invece
    // di essere ridimensionati.
    configInfo.scissor.offset = { 0, 0 };
    configInfo.scissor.extent = { width, height };

    // ==========================================
    // 3° STADIO: RASTERIZATION
    // ==========================================
    // Scompone la geometria in frammenti per ciascun pixel coperto.
    configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // Se abilitato (true), i frammenti oltre i piani di clipping (z < 0 o z > 1) vengono clampati
    // invece che scartati
    configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
    // Se abilitato (true), scarta tutte le primitive prima del rasterizer (usato se non si vuole
    // output grafico)
    configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    // FILL disegna i triangoli pieni (altre opzioni: LINE per wireframe, POINT per soli vertici)
    configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    configInfo.rasterizationInfo.lineWidth = 1.0f;
    // Cull mode: scarta le facce dei triangoli in base alla loro direzione apparente (winding
    // order) e punto di vista
    configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // Depth bias: altera i valori di profondità (usato per tecniche come shadow mapping per evitare
    // shadow acne)
    configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
    configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
    configInfo.rasterizationInfo.depthBiasClamp = 0.0f;          // Optional
    configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

    // ==========================================
    // MULTISAMPLING (MSAA)
    // ==========================================
    // Gestisce l'antialiasing lungo i bordi della geometria campionando più punti per pixel.
    // Impostato a 1 sample per pixel (MSAA disabilitato).
    configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
    configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    configInfo.multisampleInfo.minSampleShading = 1.0f;          // Optional
    configInfo.multisampleInfo.pSampleMask = nullptr;            // Optional
    configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional
    configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;      // Optional

    // ==========================================
    // COLOR BLENDING
    // ==========================================
    // Controlla come combinare il colore calcolato dal fragment shader con quello già presente nel
    // framebuffer. Maschera dei canali RGBA che possono essere scritti
    configInfo.colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    // Blending disabilitato: il nuovo colore sovrascrive direttamente il valore precedente
    configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
    configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

    configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
    configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    configInfo.colorBlendInfo.attachmentCount = 1;
    configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
    configInfo.colorBlendInfo.blendConstants[0] = 0.0f; // Optional
    configInfo.colorBlendInfo.blendConstants[1] = 0.0f; // Optional
    configInfo.colorBlendInfo.blendConstants[2] = 0.0f; // Optional
    configInfo.colorBlendInfo.blendConstants[3] = 0.0f; // Optional

    // ==========================================
    // DEPTH & STENCIL TESTING
    // ==========================================
    // Il depth buffer memorizza la profondità del frammento più vicino per ogni pixel.
    // Con LESS, il nuovo frammento viene disegnato solo se la sua profondità è minore (più vicino)
    // rispetto a quella nel buffer.
    configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.minDepthBounds = 0.0f; // Optional
    configInfo.depthStencilInfo.maxDepthBounds = 1.0f; // Optional
    configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.front = {}; // Optional
    configInfo.depthStencilInfo.back = {};  // Optional

    return configInfo;
  }
}