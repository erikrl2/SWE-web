#include "SweApp.hpp"

#include "Scenarios/TestScenario.hpp"

#include <bgfx/embedded_shader.h>
#include <bx/math.h>
#include <imgui.h>
#include <iostream>

#include "swe/fs_swe.bin.h"
#include "swe/vs_swe.bin.h"

namespace App {

  static const bgfx::EmbeddedShader shaders[] = {
    BGFX_EMBEDDED_SHADER(vs_swe),
    BGFX_EMBEDDED_SHADER(fs_swe),

    BGFX_EMBEDDED_SHADER_END()
  };

  bgfx::VertexLayout CellVertex::layout;

  void CellVertex::init() {
    layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end();
  };

  SweApp::SweApp():
    Core::Application("Swe", 1280, 720) {
    glfwSetKeyCallback(m_window, keyCallback);

    CellVertex::init();

    // u_mtx = bgfx::createUniform("...", bgfx::UniformType::...);

    bgfx::RendererType::Enum type = bgfx::getRendererType();
    bgfx::ShaderHandle vertexShader   = bgfx::createEmbeddedShader(shaders, type, "vs_swe");
    bgfx::ShaderHandle fragmentShader = bgfx::createEmbeddedShader(shaders, type, "fs_swe");
    m_program = bgfx::createProgram(vertexShader, fragmentShader, true);

    int n = 10;
    Scenarios::TestScenario scenario(n);
    int nx = n, ny = n;
    float dx = 1.0f, dy = 1.0f;

    m_height = Float2D<float>(ny + 2, nx + 2);

    for (int j = 1; j <= ny; j++) {
      for (int i = 1; i <= nx; i++) {
        float x = (i - 0.5f) * dx;
        float y = (j - 0.5f) * dy;
        m_height[j][i] = scenario.getWaterHeight(x, y);
      }
    }

    // TODO: Do scenario loading properly (and use DimensionalSplittingBlock)

  }

  SweApp::~SweApp() {
    bgfx::destroy(m_program);
    // bgfx::destroy(...);
  }

  void SweApp::updateImGui() {
    // TODO: Control uniform for color calculation in shader (min/max clipping etc)
    // TODO: Control uniform for height exaggeration
    // TODO: Control uniform for camera clipping (near/far)
  }

  static double deltaTime() {
    static double lastTime    = glfwGetTime();
    double        currentTime = glfwGetTime();
    double        deltaTime   = currentTime - lastTime;
    lastTime                  = currentTime;
    return deltaTime;
  }

  void SweApp::updateDebugText() {
    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(1, 1, 0x0f, "Current FPS: %.1f", 1.0 / deltaTime());
    bgfx::setDebug(m_toggleDebugRender ? BGFX_DEBUG_WIREFRAME : BGFX_DEBUG_TEXT);
  }

  void SweApp::update() {
    bgfx::touch(0);

    updateDebugText();

    // TODO: Don't hardcode values from scenario

    {
      // TODO: Try out perspective projection and 3d camera
      float proj[16];
      float aspect = (float) m_windowWidth / (float) m_windowHeight;
      float left = 0.0f, right = 10.0f, bottom = 0.0f, top = 10.0f; // boundary pos
      if (aspect > 1.0f) {
        left *= aspect;
        right *= aspect;
      } else {
        bottom /= aspect;
        top /= aspect;
      }
      bx::mtxOrtho(proj, left, right, bottom, top, 0.0f, 50.0f, 0, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Left);

      bgfx::setViewTransform(0, nullptr, proj);
    }

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;
    bgfx::allocTransientBuffers(&tvb, CellVertex::layout, 10 * 10, &tib, 9 * 9 * 6);

    Float2D<CellVertex> vertices(10, 10, (CellVertex*) tvb.data);
    for (int j = 0; j < 10; j++) {
      for (int i = 0; i < 10; i++) {
        float x = i + 0.5f;
        float y = j + 0.5f;
        vertices[j][i] = {x, y, m_height[j+1][i+1]};
      }
    }

    uint16_t* indices = (uint16_t*) tib.data;
    CellVertex* base = vertices.getData();
    int c = 0;

    for (int j = 0; j < 10 - 1; j++) {
      for (int i = 0; i < 10 - 1; i++) {
        int topLeft = &vertices[j][i] - base;
        int topRight = topLeft + 1;
        int bottomLeft = &vertices[j + 1][i] - base;
        int bottomRight = bottomLeft + 1;

        indices[c++] = topLeft;
        indices[c++] = topRight;
        indices[c++] = bottomLeft;
        indices[c++] = bottomLeft;
        indices[c++] = topRight;
        indices[c++] = bottomRight;
      }
    }

    bgfx::setState(BGFX_STATE_DEFAULT);
    // bgfx::setState(BGFX_STATE_WRITE_MASK | BGFX_STATE_DEPTH_TEST_LESS); // For debugging culling issues

    bgfx::setIndexBuffer(&tib);
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::submit(0, m_program);

    // bgfx::setUniform(...);

    bgfx::frame();
  }

  void SweApp::keyCallback(GLFWwindow*, int key, int, int action, int) {
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
      SweApp& app = dynamic_cast<SweApp&>(*Core::Application::get());
      app.m_toggleDebugRender = !app.m_toggleDebugRender;
    }
  }

} // namespace App

Core::Application* Core::createApplication(int, char**) { return new App::SweApp(); }
