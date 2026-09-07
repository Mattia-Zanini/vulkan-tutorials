#pragma once

#include "vulkan/vulkan_core.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace lve {

  class LveWindow {

  public:
    LveWindow(int w, int h, std::string name);
    ~LveWindow();

    // in questo modo evito che la finestra possa essere copiata
    LveWindow(const LveWindow&) = delete;
    LveWindow& operator=(const LveWindow&) = delete;

    bool shouldClose() { return glfwWindowShouldClose(window); }

    // Restituisce le dimensioni della finestra come VkExtent2D (struttura con width e height senza
    // segno). È necessario il cast esplicito a uint32_t poiché GLFW memorizza le dimensioni come
    // int con segno.
    VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }

    void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
    // Restituisce true se le dimensioni della finestra sono cambiate
    bool wasWindowResized() { return framebufferResized; }
    // Resetta il flag di resize dopo che la swap chain è stata ricreata
    void resetWindowResizedFlag() { framebufferResized = false; }

  private:
    // Callback invocata da GLFW ogni volta che le dimensioni del framebuffer della finestra cambiano
    static void framebufferResizedCallBack(GLFWwindow* window, int width, int height);
    void initWindow();

    // width e height non sono più const poiché la finestra può essere ridimensionata a runtime
    int width;
    int height;
    bool framebufferResized = false;

    std::string windowName;
    GLFWwindow* window;
  };

}