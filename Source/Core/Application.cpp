#include "Application.hpp"

#include <bgfx/platform.h>
#include <GLFW/glfw3.h>

#include "ImGui/ImGuiBgfx.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#else
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
    m_windowSize(width, height) {
    if (s_app) {
      assert(false);
      return;
    }
    s_app = this;

    glfwSetErrorCallback([](int error, const char* description) { std::cerr << "GLFW error " << error << ": " << description << std::endl; });

    if (!glfwInit()) {
      std::cerr << "Failed to initialize GLFW" << std::endl;
      return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_windowSize.x, m_windowSize.y, m_title.c_str(), NULL, NULL);
    if (!m_window) {
      std::cerr << "Failed to create GLFW window" << std::endl;
      assert(false);
      return;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, emscriptenResizeCallback);
#else
    glfwSetFramebufferSizeCallback(m_window, glfwFramebufferSizeCallback);
#endif

    glfwSetKeyCallback(m_window, glfwKeyCallback);
    glfwSetScrollCallback(m_window, glfwScrollCallback);
    glfwSetDropCallback(m_window, glfwDropCallback);

    bgfx::renderFrame(); // Indicates bgfx to use single threaded rendering

    bgfx::Init bgfxInit;

#if BX_PLATFORM_LINUX
    bgfxInit.platformData.nwh  = (void*)uintptr_t(glfwGetX11Window(m_window));
    bgfxInit.platformData.ndt  = glfwGetX11Display();
    bgfxInit.platformData.type = bgfx::NativeWindowHandleType::Default;
    bgfxInit.type              = bgfx::RendererType::OpenGL;
#elif BX_PLATFORM_OSX
    bgfxInit.platformData.nwh  = glfwGetCocoaWindow(m_window);
    bgfxInit.platformData.ndt  = NULL;
    bgfxInit.platformData.type = bgfx::NativeWindowHandleType::Default;
    bgfxInit.type              = bgfx::RendererType::Metal;
#elif BX_PLATFORM_WINDOWS
    bgfxInit.platformData.nwh  = glfwGetWin32Window(m_window);
    bgfxInit.platformData.ndt  = NULL;
    bgfxInit.platformData.type = bgfx::NativeWindowHandleType::Default;
    bgfxInit.type              = bgfx::RendererType::Direct3D11;
#elif __EMSCRIPTEN__
    bgfxInit.platformData.nwh = (void*)"#canvas";
    bgfxInit.type             = bgfx::RendererType::OpenGLES;
#endif

#ifdef __EMSCRIPTEN__
    double w, h;
    emscripten_get_element_css_size("#canvas", &w, &h);
    m_windowSize.x = (int)w;
    m_windowSize.y = (int)h;
#else
    glfwGetWindowSize(m_window, &m_windowSize.x, &m_windowSize.y);
#endif
    bgfxInit.resolution.width  = (uint32_t)m_windowSize.x;
    bgfxInit.resolution.height = (uint32_t)m_windowSize.y;
    bgfxInit.resolution.reset  = m_resetFlags;

    if (!bgfx::init(bgfxInit)) {
      std::cerr << "Failed to initialize bgfx" << std::endl;
      glfwDestroyWindow(m_window);
      glfwTerminate();
      assert(false);
      return;
    }

    bgfx::setDebug(m_debugFlags);
    bgfx::setViewClear(m_mainView, m_clearFlags);
    bgfx::setViewRect(s_app->m_mainView, 0, 0, bgfx::BackbufferRatio::Equal);

    ImGuiBgfx::createImGuiContext(m_window);
  }

  Application::~Application() {
    ImGuiBgfx::destroyImGuiContext();

    bgfx::shutdown();

    glfwDestroyWindow(m_window);
    glfwTerminate();
  }

#ifdef __EMSCRIPTEN__
  void Application::emscriptenMainLoop() {
    glfwPollEvents();

    if (glfwWindowShouldClose(s_app->m_window)) {
      emscripten_cancel_main_loop();
      return;
    }

    float dt = ImGui::GetIO().DeltaTime;

    ImGuiBgfx::beginImGuiFrame();
    s_app->updateImGui(dt);
    ImGuiBgfx::endImGuiFrame();

    s_app->update(dt);
  }
#endif

  void Application::run() {
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(emscriptenMainLoop, 0, true);
#else
    while (!glfwWindowShouldClose(m_window)) {
      glfwPollEvents();

      float dt = ImGui::GetIO().DeltaTime;

      ImGuiBgfx::beginImGuiFrame();
      updateImGui(dt);
      ImGuiBgfx::endImGuiFrame();

      update(dt);
    }
#endif
  }

#ifdef __EMSCRIPTEN__
  bool Application::emscriptenResizeCallback(int, const EmscriptenUiEvent* e, void*) {
    int width  = (int)e->windowInnerWidth;
    int height = (int)e->windowInnerHeight;

    s_app->m_windowSize.x = width;
    s_app->m_windowSize.y = height;

    bgfx::reset((uint32_t)width, (uint32_t)height, s_app->m_resetFlags);
    bgfx::setViewRect(s_app->m_mainView, 0, 0, bgfx::BackbufferRatio::Equal);

    s_app->onResize(width, height);

    return true;
  }
#endif

  void Application::glfwFramebufferSizeCallback(GLFWwindow*, int width, int height) {
    s_app->m_windowSize.x = width;
    s_app->m_windowSize.y = height;
    bgfx::reset((uint32_t)width, (uint32_t)height, s_app->m_resetFlags);
    bgfx::setViewRect(s_app->m_mainView, 0, 0, bgfx::BackbufferRatio::Equal);

    s_app->onResize(width, height);
  }

  void Application::glfwKeyCallback(GLFWwindow*, int key, int, int action, int) {
    if (ImGui::GetIO().WantTextInput)
      return;

    if (action == GLFW_PRESS) {
      s_app->onKeyPressed(key);
    }
  }

  void Application::glfwScrollCallback(GLFWwindow*, double xoffset, double yoffset) { s_app->onMouseScrolled(xoffset, yoffset); }

  void Application::glfwDropCallback(GLFWwindow*, int count, const char** paths) {
    for (int i = 0; i < count; ++i) {
      s_app->onFileDropped(paths[i]);
    }
  }

} // namespace Core
