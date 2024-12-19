#include "Application.hpp"

#include <bgfx/platform.h>

#include "ImGui/ImGuiBgfx.hpp"

#ifndef __EMSCRIPTEN__
#if BX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BX_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>
#endif

#include <iostream>

namespace Core {

  Application::Application(const std::string& title, int width, int height):
    m_title(title),
    m_windowWidth(width),
    m_windowHeight(height),
    m_mainView(0) {
    if (s_app) {
      assert(false);
      return;
    }
    s_app = this;

    glfwSetErrorCallback([](int error, const char* description) {
      std::cerr << "GLFW error " << error << ": " << description << std::endl;
    });

    if (!glfwInit()) {
      std::cerr << "Failed to initialize GLFW" << std::endl;
      return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, m_title.c_str(), NULL, NULL);
    if (!m_window) {
      std::cerr << "Failed to create GLFW window" << std::endl;
      return;
    }

    glfwSetFramebufferSizeCallback(m_window, windowSizeCallback); // works only for desktop

    bgfx::renderFrame(); // signals bgfx not to create a render thread

    bgfx::Init bgfxInit;

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    bgfxInit.platformData.nwh = (void*)glfwGetX11Window(m_window);
#elif BX_PLATFORM_OSX
    bgfxInit.platformData.nwh = (void*)glfwGetCocoaWindow(window);
#elif BX_PLATFORM_WINDOWS
    bgfxInit.platformData.nwh = (void*)glfwGetWin32Window(window);
#elif __EMSCRIPTEN__
    bgfxInit.platformData.nwh = (void*)"#canvas";
#endif

    glfwGetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
    bgfxInit.resolution.width  = (uint32_t)m_windowWidth;
    bgfxInit.resolution.height = (uint32_t)m_windowHeight;
    bgfxInit.resolution.reset  = BGFX_RESET_VSYNC;

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

#ifdef __EMSCRIPTEN__
  EM_JS(int, getCanvasWidth, (), { return document.getElementById('canvas').width; });
  EM_JS(int, getCanvasHeight, (), { return document.getElementById('canvas').height; });

  void Application::emscriptenMainLoop() {
    glfwPollEvents();

    if (glfwWindowShouldClose(s_app->m_window)) {
      emscripten_cancel_main_loop();
      return;
    }

    int width  = getCanvasWidth();
    int height = getCanvasHeight();
    if (width != s_app->m_windowWidth || height != s_app->m_windowHeight) {
      glfwSetWindowSize(s_app->m_window, width, height);
      windowSizeCallback(s_app->m_window, width, height);
    }

    ImGuiBgfx::beginImGuiFrame();
    s_app->updateImGui();
    ImGuiBgfx::endImGuiFrame();

    s_app->update();
  }
#endif

  void Application::run() {
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(emscriptenMainLoop, 0, true);
#else
    while (!glfwWindowShouldClose(m_window)) {
      glfwPollEvents();

      ImGuiBgfx::beginImGuiFrame();
      updateImGui();
      ImGuiBgfx::endImGuiFrame();

      update();
    }
#endif
  }

  void Application::windowSizeCallback(GLFWwindow*, int width, int height) {
    s_app->m_windowWidth  = width;
    s_app->m_windowHeight = height;
    bgfx::reset((uint32_t)width, (uint32_t)height, BGFX_RESET_VSYNC);
    bgfx::setViewRect(s_app->m_mainView, 0, 0, bgfx::BackbufferRatio::Equal);
  }

} // namespace Core
