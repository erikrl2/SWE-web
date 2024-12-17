#include "SweApp.hpp"

#include <bgfx/embedded_shader.h>
#include <bx/math.h>
#include <imgui.h>
#include <iostream>

#include "swe/fs_cubes.bin.h"
#include "swe/vs_cubes.bin.h"

namespace App {

  struct CubeVertex {
    float    x, y, z;
    uint32_t abrg;
  };

  static const bgfx::EmbeddedShader shaders[] = {
    BGFX_EMBEDDED_SHADER(vs_cubes),
    BGFX_EMBEDDED_SHADER(fs_cubes),

    BGFX_EMBEDDED_SHADER_END()
  };

  static CubeVertex cubeVertices[] = {
    {-1.0f, 1.0f, 1.0f, 0xff000000},
    {1.0f, 1.0f, 1.0f, 0xff0000ff},
    {-1.0f, -1.0f, 1.0f, 0xff00ff00},
    {1.0f, -1.0f, 1.0f, 0xff00ffff},
    {-1.0f, 1.0f, -1.0f, 0xffff0000},
    {1.0f, 1.0f, -1.0f, 0xffff00ff},
    {-1.0f, -1.0f, -1.0f, 0xffffff00},
    {1.0f, -1.0f, -1.0f, 0xffffffff},
  };

  static const uint16_t cubeIndices[] = {
    0, 1, 2, 1, 3, 2, 4, 6, 5, 5, 6, 7, 0, 2, 4, 4, 2, 6, 1, 5, 3, 5, 7, 3, 0, 4, 1, 4, 5, 1, 2, 3, 6, 6, 3, 7,
  };

  SweApp::SweApp():
    Core::Application("Demo", 1280, 720) {
    glfwSetKeyCallback(m_window, keyCallback);
    // glfwSetCursorPosCallback(m_window, cursorPosCallback);
    // glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    // glfwSetScrollCallback(m_window, scrollCallback);
    // glfwSetDropCallback(m_window, dropFileCallback);

    bgfx::VertexLayout cubeVertexLayout;
    cubeVertexLayout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true).end();
    m_vertexBuffer = bgfx::createVertexBuffer(bgfx::makeRef(cubeVertices, sizeof(cubeVertices)), cubeVertexLayout);
    m_indexBuffer  = bgfx::createIndexBuffer(bgfx::makeRef(cubeIndices, sizeof(cubeIndices)));

    bgfx::RendererType::Enum type = bgfx::getRendererType();

    bgfx::ShaderHandle vertexShader   = bgfx::createEmbeddedShader(shaders, type, "vs_cubes");
    bgfx::ShaderHandle fragmentShader = bgfx::createEmbeddedShader(shaders, type, "fs_cubes");

    m_program = bgfx::createProgram(vertexShader, fragmentShader, true);
  }

  SweApp::~SweApp() {
    bgfx::destroy(m_indexBuffer);
    bgfx::destroy(m_vertexBuffer);
  }

  void SweApp::updateImGui() { ImGui::ShowDemoWindow(); }

  void SweApp::update() {
    static unsigned counter = 0;

    // Calculate fps
    static double lastTime    = glfwGetTime();
    double        currentTime = glfwGetTime();
    double        deltaTime   = currentTime - lastTime;
    lastTime                  = currentTime;
    double fps                = 1.0 / deltaTime;

    bgfx::touch(m_mainView);

    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(1, 1, 0x0f, "Press Tab to toggle stats.");
    bgfx::dbgTextPrintf(1, 3, 0x0f, "Current FPS: %.0f", fps);
    bgfx::setDebug(m_showStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);

    const bx::Vec3 at  = {0.0f, 0.0f, 0.0f};
    const bx::Vec3 eye = {0.0f, 0.0f, -5.0f};

    float view[16];
    bx::mtxLookAt(view, eye, at);
    float proj[16];
    bx::mtxProj(proj, 60.0f, float(m_width) / float(m_height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
    bgfx::setViewTransform(0, view, proj);

    float mtx[16];
    bx::mtxRotateXY(mtx, counter * 0.01f, counter * 0.01f);
    bgfx::setTransform(mtx);

    bgfx::setVertexBuffer(0, m_vertexBuffer);
    bgfx::setIndexBuffer(m_indexBuffer);

    bgfx::submit(0, m_program);

    bgfx::frame();

    counter++;
  }

  void SweApp::keyCallback(GLFWwindow*, int key, int, int action, int) {
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
      SweApp& app     = dynamic_cast<SweApp&>(*Core::Application::get());
      app.m_showStats = !app.m_showStats;
    }
  }

  // void SweApp::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {}
  // void SweApp::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {}
  // void SweApp::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {}
  // void SweApp::dropFileCallback(GLFWwindow* window, int count, const char** paths) {}

} // namespace App

Core::Application* Core::createApplication(int, char**) { return new App::SweApp(); }
