#pragma once

#include "lve_device.hpp"
#include "vulkan/vulkan_core.h"

#include <cstdint>
#include <string>
#include <vector>

namespace lve {
  // Struct per configurare i vari stadi a funzione fissa (Fixed Function) della pipeline grafica
  struct PipelineConfigInfo {
    // Costruttore di default esplicito necessario poiché i costruttori di copia sono stati eliminati
    PipelineConfigInfo() = default;
    PipelineConfigInfo(const PipelineConfigInfo&) = delete;
    PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

    // Descrizioni di binding e attributi dei vertici configurabili (possono essere lasciati vuoti per pipeline senza vertex buffer, es. billboard)
    std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    VkPipelineViewportStateCreateInfo viewportInfo;
    // 1° stadio (Input Assembler): raggruppa la lista di vertici grezzi in geometrie (es. triangoli, linee, punti)
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    // 3° stadio (Rasterizer): scompone la geometria in frammenti per ogni pixel e gestisce culling, fill mode e depth clamp
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    // Configura il multisampling (MSAA) per l'antialiasing lungo i bordi della geometria
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    // Configurazione del blending per il singolo attachment (come mescolare il colore del frammento col framebuffer)
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    // Configurazione globale del color blending
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    // Configura il depth testing (scarta i frammenti coperti da oggetti più vicini usando il depth buffer) e lo stencil test
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;

    // Elenco degli stati dinamici abilitati (es. Viewport e Scissor modificabili nel command buffer senza ricreare la pipeline)
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;

    // Risorse esterne (uniform buffer, push constants, descriptor set) accessibili dagli shader
    VkPipelineLayout pipelineLayout = nullptr;
    // Render pass e relativo subpass in cui verrà eseguita la pipeline
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
  };

  class LvePipeline {
  public:
    LvePipeline(
      LveDevice& device,
      const std::string& vertFilePath,
      const std::string& fragFilePath,
      const PipelineConfigInfo& configInfo);
    ~LvePipeline();

    LvePipeline(const LvePipeline&) = delete;
    LvePipeline& operator=(const LvePipeline&) = delete;

    // Lega la pipeline grafica al command buffer per le successive operazioni di disegno
    void bind(VkCommandBuffer commandBuffer);
    static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
    // Configura l'attachment di color blend per abilitare l'alpha blending tradizionale (trasparenza)
    static void enableAlphaBlending(PipelineConfigInfo& configInfo);

  private:
    static std::vector<char> readFile(const std::string& filePath);
    void createGraphicsPipeline(const std::string& vertFilePath, const std::string& fragFilePath, const PipelineConfigInfo& configInfo);
    void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

    LveDevice& lveDevice;
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
  };
}