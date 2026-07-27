#include "lve_pipeline.hpp"

#include <fstream>
#include <iostream>
#include <fmt/format.h>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace lve {
    LvePipeline::LvePipeline(
        LveDevice& device,
        const std::string& vertFilePath,
        const std::string& fragFilePath,
        const PipelineConfigInfo& configInfo) : lveDevice{ device } {
        createGraphicsPipeline(vertFilePath, fragFilePath, configInfo);
    }

    LvePipeline::~LvePipeline() {}

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

    void LvePipeline::createGraphicsPipeline(
        const std::string& vertFilePath,
        const std::string& fragFilePath,
        const PipelineConfigInfo& configInfo)
    {
        auto vertCode = readFile(vertFilePath);
        auto fragCode = readFile(fragFilePath);

        spdlog::debug("Vertex Shader Code Size: {}", vertCode.size());
        spdlog::debug("Fragment Shader Code Size: {}", fragCode.size());
    }

    void LvePipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) {
        VkShaderModuleCreateInfo createinfo
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const u_int32_t*>(code.data())
        };

        if (vkCreateShaderModule(lveDevice.device(), &createinfo, nullptr, shaderModule) != VK_SUCCESS) {
            spdlog::error("failed to create shader module");
            throw std::runtime_error("failed to create shader module");
        }
    }

    PipelineConfigInfo LvePipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height) {
        PipelineConfigInfo configInfo{};

        return configInfo;
    }
}