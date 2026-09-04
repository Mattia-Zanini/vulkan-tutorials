#pragma once

#include "lve_device.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace lve {
  // Struct per configurare i vari stadi a funzione fissa (Fixed Function) della pipeline grafica
  struct PipelineConfigInfo {
    // Viewport: descrive la trasformazione dalle coordinate normalizzate di output [-1, 1] ai pixel
    // dell'immagine target
    VkViewport viewport;

    // Scissor: definisce un rettangolo di ritaglio; qualunque pixel al di fuori viene scartato
    VkRect2D scissor;

    // 1° stadio (Input Assembler): raggruppa la lista di vertici grezzi in geometrie (es.
    // triangoli, linee, punti)
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;

    // 3° stadio (Rasterizer): scompone la geometria in frammenti per ogni pixel e gestisce culling,
    // fill mode e depth clamp
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;

    // Configura il multisampling (MSAA) per l'antialiasing lungo i bordi della geometria
    VkPipelineMultisampleStateCreateInfo multisampleInfo;

    // Configurazione del blending per il singolo attachment (come mescolare il colore del frammento
    // col framebuffer)
    VkPipelineColorBlendAttachmentState colorBlendAttachment;

    // Configurazione globale del color blending
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;

    // Configura il depth testing (scarta i frammenti coperti da oggetti più vicini usando il depth
    // buffer) e lo stencil test
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;

    // Risorse esterne (uniform buffer, push constants, descriptor set) accessibili dagli shader
    VkPipelineLayout pipelineLayout = nullptr;

    // Render pass e relativo subpass in cui verrà eseguita la pipeline
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
  };

  class LvePipeline {
  public:
    LvePipeline(LveDevice& device, const std::string& vertFilePath, const std::string& fragFilePath,
                const PipelineConfigInfo& configInfo);
    ~LvePipeline();

    LvePipeline(const LvePipeline&) = delete;
    LvePipeline& operator=(const LvePipeline&) = delete;

    static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height);

  private:
    static std::vector<char> readFile(const std::string& filePath);
    void createGraphicsPipeline(const std::string& vertFilePath, const std::string& fragFilePath,
                                const PipelineConfigInfo& configInfo);
    void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

    LveDevice& lveDevice;
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
  };
}