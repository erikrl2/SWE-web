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
    m_windowSize(width, height),
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
    m_window = glfwCreateWindow(m_windowSize.x, m_windowSize.y, m_title.c_str(), NULL, NULL);
    if (!m_window) {
      std::cerr << "Failed to create GLFW window" << std::endl;
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

    // #if BGFX_CONFIG_MULTITHREADED
    //     bgfx::renderFrame(); // signals bgfx not to create a render thread
    // #endif

    bgfx::Init bgfxInit;

#if BX_PLATFORM_LINUX
    bgfxInit.platformData.nwh  = (void*)uintptr_t(glfwGetX11Window(m_window));
    bgfxInit.platformData.ndt  = glfwGetX11Display();
    bgfxInit.platformData.type = bgfx::NativeWindowHandleType::Default;
    bgfxInit.type              = bgfx::RendererType::Vulkan;
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
    double devicePixelRatio = emscripten_get_device_pixel_ratio();
    double w, h;
    emscripten_get_element_css_size(".app", &w, &h);
    w *= devicePixelRatio;
    h *= devicePixelRatio;
    emscripten_set_canvas_element_size("#canvas", (int)w, (int)h);
    glfwSetWindowSize(s_app->m_window, (int)w, (int)h);
#endif

    glfwGetWindowSize(m_window, &m_windowSize.x, &m_windowSize.y);
    bgfxInit.resolution.width  = (uint32_t)m_windowSize.x;
    bgfxInit.resolution.height = (uint32_t)m_windowSize.y;
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
    double devicePixelRatio = emscripten_get_device_pixel_ratio();

    int width  = (int)(e->windowInnerWidth * devicePixelRatio);
    int height = (int)(e->windowInnerHeight * devicePixelRatio);

    s_app->m_windowSize.x = width;
    s_app->m_windowSize.y = height;

    emscripten_set_canvas_element_size("#canvas", width, height);
    glfwSetWindowSize(s_app->m_window, width, height);

    bgfx::reset((uint32_t)width, (uint32_t)height, BGFX_RESET_VSYNC);
    bgfx::setViewRect(s_app->m_mainView, 0, 0, bgfx::BackbufferRatio::Equal);

    s_app->onResize(width, height);

    return true;
  }
#endif

  void Application::glfwFramebufferSizeCallback(GLFWwindow*, int width, int height) {
    s_app->m_windowSize.x = width;
    s_app->m_windowSize.y = height;
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

  void Application::glfwScrollCallback(GLFWwindow*, double xoffset, double yoffset) { s_app->onMouseScrolled(xoffset, yoffset); }

  void Application::glfwDropCallback(GLFWwindow*, int count, const char** paths) {
    for (int i = 0; i < count; ++i) {
      s_app->onFileDropped(paths[i]);
    }
  }

} // namespace Core
