#include "Application.hpp"

#include <bgfx/platform.h>
#include <bx/platform.h>

#include "ImGui/ImGuiBgfx.hpp"

#if BX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BX_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>
#include <iostream>

namespace Core {

  Application::Application(const std::string& title, int width, int height):
    m_title(title),
    m_width(width),
    m_height(height),
    m_mainView(0) {
    if (s_app) {
      assert(false);
      return;
    }
    s_app = this;

    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
      std::cerr << "Failed to initialize GLFW" << std::endl;
      return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL);
    if (!m_window) {
      std::cerr << "Failed to create GLFW window" << std::endl;
      return;
    }

    // Set useful event callbacks
    glfwSetWindowSizeCallback(m_window, windowSizeCallback);

    bgfx::renderFrame(); // signals bgfx not to create a render thread

    bgfx::Init bgfxInit;

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    bgfxInit.platformData.nwh = (void*)glfwGetX11Window(m_window);
#elif BX_PLATFORM_OSX
    bgfxInit.platformData.nwh = (void*)glfwGetCocoaWindow(window);
#elif BX_PLATFORM_WINDOWS
    bgfxInit.platformData.nwh = (void*)glfwGetWin32Window(window);
#endif

    glfwGetWindowSize(m_window, &m_width, &m_height);
    bgfxInit.resolution.width  = (uint32_t)m_width;
    bgfxInit.resolution.height = (uint32_t)m_height;
    bgfxInit.resolution.reset  = BGFX_RESET_VSYNC; // This does not seem to work...

    if (!bgfx::init(bgfxInit)) {
      std::cerr << "Failed to initialize bgfx" << std::endl;
      return;
    }

    bgfx::setViewClear(m_mainView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF);
    bgfx::setViewRect(m_mainView, 0, 0, bgfx::BackbufferRatio::Equal);

    ImGuiBgfx::createImGuiContext(m_window);
  }

  Application::~Application() {
    ImGuiBgfx::destroyImGuiContext();

    bgfx::shutdown();

    glfwDestroyWindow(m_window);
    glfwTerminate();
  }

  void Application::run() {
    while (!glfwWindowShouldClose(m_window)) {
      glfwPollEvents();

      ImGuiBgfx::beginImGuiFrame();
      updateImGui();
      ImGuiBgfx::endImGuiFrame();

      update();
    }
  }

  void Application::glfwErrorCallback(int error, const char* description) { std::cerr << "GLFW error " << error << ": " << description << std::endl; }

  void Application::windowSizeCallback(GLFWwindow*, int width, int height) {
    s_app->m_width  = width;
    s_app->m_height = height;
    bgfx::reset((uint32_t)width, (uint32_t)height, BGFX_RESET_VSYNC);
    bgfx::setViewRect(s_app->m_mainView, 0, 0, bgfx::BackbufferRatio::Equal);
  }

} // namespace Core
