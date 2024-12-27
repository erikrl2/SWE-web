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

    glfwSetErrorCallback([](int error, const char* description) { std::cerr << "GLFW error " << error << ": " << description << std::endl; });

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

#ifdef __EMSCRIPTEN__
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, emscriptenResizeCallback);
#else
    glfwSetFramebufferSizeCallback(m_window, glfwFramebufferSizeCallback);
#endif

    glfwSetKeyCallback(m_window, glfwKeyCallback);
    glfwSetDropCallback(m_window, glfwDropCallback);

    bgfx::renderFrame(); // signals bgfx not to create a render thread

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
    bgfxInit.type              = bgfx::RendererType::OpenGL;
#elif BX_PLATFORM_WINDOWS
    bgfxInit.platformData.nwh  = glfwGetWin32Window(m_window);
    bgfxInit.platformData.ndt  = NULL;
    bgfxInit.platformData.type = bgfx::NativeWindowHandleType::Default;
    bgfxInit.type              = bgfx::RendererType::OpenGL;
#elif __EMSCRIPTEN__
    bgfxInit.platformData.nwh = (void*)"#canvas";
    bgfxInit.type             = bgfx::RendererType::OpenGLES;
#endif

#ifdef __EMSCRIPTEN__
    double w, h;
    emscripten_get_element_css_size(".app", &w, &h);
    emscripten_set_canvas_element_size("#canvas", (int)w, (int)h);
    glfwSetWindowSize(s_app->m_window, (int)w, (int)h);
#endif

    glfwGetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
    bgfxInit.resolution.width  = (uint32_t)m_windowWidth;
    bgfxInit.resolution.height = (uint32_t)m_windowHeight;
    bgfxInit.resolution.reset  = BGFX_RESET_VSYNC;

    if (!bgfx::init(bgfxInit)) {
      std::cerr << "Failed to initialize bgfx" << std::endl;
      return;
    }

    bgfx::setViewClear(m_mainView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH);
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
  void Application::emscriptenMainLoop() {
    glfwPollEvents();

    if (glfwWindowShouldClose(s_app->m_window)) {
      emscripten_cancel_main_loop();
      return;
    }

    static double lastTime = glfwGetTime();
    double        now      = glfwGetTime();
    double        dt       = now - lastTime;
    lastTime               = now;

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

      static double lastTime = glfwGetTime();
      double        now      = glfwGetTime();
      double        dt       = now - lastTime;
      lastTime               = now;

      ImGuiBgfx::beginImGuiFrame();
      updateImGui(dt);
      ImGuiBgfx::endImGuiFrame();

      update(dt);
    }
#endif
  }

#ifdef __EMSCRIPTEN__
  EM_BOOL Application::emscriptenResizeCallback(int, const EmscriptenUiEvent* e, void*) {
    int width  = e->windowInnerWidth;
    int height = e->windowInnerHeight;

    s_app->m_windowWidth  = width;
    s_app->m_windowHeight = height;

    emscripten_set_canvas_element_size("#canvas", width, height);
    glfwSetWindowSize(s_app->m_window, width, height);

    bgfx::reset((uint32_t)width, (uint32_t)height, BGFX_RESET_VSYNC);
    bgfx::setViewRect(s_app->m_mainView, 0, 0, bgfx::BackbufferRatio::Equal);

    s_app->onResize(width, height);

    return EM_TRUE;
  }
#endif

  void Application::glfwFramebufferSizeCallback(GLFWwindow*, int width, int height) {
    s_app->m_windowWidth  = width;
    s_app->m_windowHeight = height;
    bgfx::reset((uint32_t)width, (uint32_t)height, BGFX_RESET_VSYNC);
    bgfx::setViewRect(s_app->m_mainView, 0, 0, bgfx::BackbufferRatio::Equal);

    s_app->onResize(width, height);
  }

  void Application::glfwKeyCallback(GLFWwindow*, int key, int, int action, int) {
    if (ImGui::GetIO().WantCaptureKeyboard)
      return;

    if (action == GLFW_PRESS) {
      s_app->onKeyPressed(key);
    }
  }

  void Application::glfwDropCallback(GLFWwindow*, int count, const char** paths) {
    for (int i = 0; i < count; ++i) {
      s_app->onFileDropped(paths[i]);
    }
  }

} // namespace Core
