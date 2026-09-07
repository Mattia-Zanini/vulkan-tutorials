#include "lve_window.hpp"
#include "GLFW/glfw3.h"

#include <spdlog/spdlog.h>

namespace lve {
  LveWindow::LveWindow(int w, int h, std::string name) : width{ w }, height{ h }, windowName{ name } { initWindow(); }

  LveWindow::~LveWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void LveWindow::framebufferResizedCallBack(GLFWwindow* window, int width, int height) {
    // Recupera il puntatore all'istanza LveWindow precedentemente associata alla finestra GLFW
    auto lveWindow = reinterpret_cast<LveWindow*>(glfwGetWindowUserPointer(window));
    lveWindow->framebufferResized = true;
    lveWindow->width = width;
    lveWindow->height = height;
  }

  void LveWindow::initWindow() {
    glfwInit();
    // disabilito la creazione di un contesto OpenGL perchè andrò ad utilizzare vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Abilita il ridimensionamento della finestra
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    // Associa questa istanza di LveWindow alla finestra GLFW per poterla recuperare nella callback
    glfwSetWindowUserPointer(window, this);
    // Registra la callback di GLFW per intercettare il ridimensionamento del framebuffer
    glfwSetFramebufferSizeCallback(window, framebufferResizedCallBack);
  }

  void LveWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
      spdlog::error("failed to create window surface");
      throw std::runtime_error("failed to create window surface");
    }
  }
}