#include "lve_window.hpp"

#include <spdlog/spdlog.h>

namespace lve {
    LveWindow::LveWindow(int w, int h, std::string name) : width{ w }, height{ h }, windowName{ name } {
        initWindow();
    }

    LveWindow::~LveWindow() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void LveWindow::initWindow() {
        glfwInit();
        // disabilito la creazione di un contesto OpenGL perchè andrò ad utilizzare vulkan
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // disabilito il resize della finestra in quanto dovrà essere trattato in maniera speciale
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    }

    void LveWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
            spdlog::error("failed to create window surface");
            throw std::runtime_error("failed to create window surface");
        }
    }
}