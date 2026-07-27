#include "lve_pipeline.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace lve {
    LvePipeline::LvePipeline(const std::string& vertFilePath, const std::string& fragFilePath) {
        createGraphicsPipeline(vertFilePath, fragFilePath);
    }

    std::vector<char> LvePipeline::readFile(const std::string& filePath) {
        // std::ios::ate -> vado alla fine del file, utile per ottenenere la dimensione del file
        // std::ios::binary -> leggo il file come stream binario
        std::ifstream file{ filePath, std::ios::ate | std::ios::binary };

        if (!file.is_open()) {
            spdlog::error("failed to open file: {}", filePath);
            throw std::runtime_error("failed to open file: " + filePath);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    void LvePipeline::createGraphicsPipeline(const std::string& vertFilePath, const std::string& fragFilePath) {
        auto vertCode = readFile(vertFilePath);
        auto fragCode = readFile(fragFilePath);

        spdlog::debug("Vertex Shader Code Size: {}", vertCode.size());
        spdlog::debug("Fragment Shader Code Size: {}", fragCode.size());
    }
}