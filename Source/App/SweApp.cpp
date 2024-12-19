#include "SweApp.hpp"

#include <algorithm>
#include <bgfx/embedded_shader.h>
#include <bx/math.h>
#include <imgui.h>
#include <iostream>

#include "Blocks/DimensionalSplitting.hpp"
#include "Scenarios/ArtificialTsunamiScenario.hpp"
#include "Scenarios/TestScenario.hpp"
#include "swe/fs_swe.bin.h"
#include "swe/vs_swe.bin.h"

namespace App {

  static const bgfx::EmbeddedShader shaders[] = {
    BGFX_EMBEDDED_SHADER(vs_swe),
    BGFX_EMBEDDED_SHADER(fs_swe),

    BGFX_EMBEDDED_SHADER_END()
  };

  bgfx::VertexLayout CellVertex::layout;

  void CellVertex::init() { layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end(); };

  enum UtilUniformIndex {
    hMin,
    hMax,
    heightExag,
  }; // Has to match order in shader

  SweApp::SweApp():
    Core::Application("Swe", 1280, 720) {
    bgfx::RendererType::Enum type           = bgfx::getRendererType();
    bgfx::ShaderHandle       vertexShader   = bgfx::createEmbeddedShader(shaders, type, "vs_swe");
    bgfx::ShaderHandle       fragmentShader = bgfx::createEmbeddedShader(shaders, type, "fs_swe");
    m_program                               = bgfx::createProgram(vertexShader, fragmentShader, true);

    CellVertex::init();

    u_color = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
    u_util  = bgfx::createUniform("u_util", bgfx::UniformType::Vec4);

    loadBlock(ScenarioType::Test, 10, 10);
  }

  SweApp::~SweApp() {
    delete m_block;
    bgfx::destroy(m_program);
    bgfx::destroy(u_color);
    bgfx::destroy(u_util);
  }

  static std::string scenarioTypeToString(ScenarioType type);

  void SweApp::loadBlock(ScenarioType scenarioType, int nx, int ny) {
    Scenarios::Scenario* scenario = nullptr;
    switch (scenarioType) {
    case ScenarioType::ArtificialTsunami:
      // TODO: Implement maxSimulationTime and boundaryType selection
      scenario = new Scenarios::ArtificialTsunamiScenario(100.0, BoundaryType::Wall);
      break;
    default:
      scenario = new Scenarios::TestScenario(nx);
      break;
    }

    RealType left   = scenario->getBoundaryPos(BoundaryEdge::Left);
    RealType right  = scenario->getBoundaryPos(BoundaryEdge::Right);
    RealType bottom = scenario->getBoundaryPos(BoundaryEdge::Bottom);
    RealType top    = scenario->getBoundaryPos(BoundaryEdge::Top);

    RealType dx = (right - left) / nx;
    RealType dy = (top - bottom) / ny;

    std::cout << "Loading block with scenario: " << scenarioTypeToString(scenarioType) << std::endl;
    std::cout << "  nx: " << nx << ", ny: " << ny << std::endl;
    std::cout << "  dx: " << dx << ", dy: " << dy << std::endl;
    std::cout << "  Left: " << left << ", Right: " << right << ", Bottom: " << bottom << ", Top: " << top << std::endl;

    if (m_block) {
      delete m_block;
    }
    m_block = new Blocks::DimensionalSplittingBlock(nx, ny, dx, dy);
    m_block->initialiseScenario(left, bottom, *scenario);
    m_block->setGhostLayer();
    delete scenario;

    // TODO: Do min/max = -0.1/0.1 later when using h+b
    const RealType* h  = m_block->getWaterHeight().getData();
    auto [min, max]    = std::minmax_element(h, h + (m_block->getNx() + 2) * (m_block->getNy() + 2));
    m_util[hMin]       = (float)*min;
    m_util[hMax]       = (float)*max;
    m_util[heightExag] = 1.0f;
  }

  void SweApp::updateImGui() {
    ImGui::SetNextWindowPos(ImVec2(20, m_windowHeight - 320), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::ColorEdit4("Color", m_color, ImGuiColorEditFlags_NoAlpha);

    ImGui::Separator();
    ImGui::DragFloat("H Min", &m_util[hMin], 0.1f);
    ImGui::DragFloat("H Max", &m_util[hMax], 0.1f);

    ImGui::Separator();
    ImGui::DragFloat("Near Clip", &m_cameraClipping[0]);
    ImGui::DragFloat("Far Clip", &m_cameraClipping[1]);

    ImGui::Separator();
    ImGui::DragFloat("Height Exaggeration", &m_util[heightExag], 0.01f, 0.0f, 100.0f);

    ImGui::Separator();
    if (ImGui::Button("Toogle debug stats"))
      m_debugFlags ^= BGFX_DEBUG_STATS;
    if (ImGui::Button("Toogle wireframe render"))
      m_debugFlags ^= BGFX_DEBUG_WIREFRAME;

    static bool scenarioSelectionOpen = false;

    ImGui::Separator();
    if (ImGui::Button("Select Scenario")) {
      scenarioSelectionOpen = true;
    }
    ImGui::Text("Simulation time: %.2f", m_simulationTime);

    ImGui::Separator();
    if (ImGui::Button("Exit")) {
      glfwSetWindowShouldClose(m_window, 1);
    }
    ImGui::End();

    if (scenarioSelectionOpen) {
      ImGui::SetNextWindowPos(ImVec2(m_windowWidth / 2 - 100, m_windowHeight / 2 - 100), ImGuiCond_FirstUseEver);
      ImGui::Begin("Scenario Selection", &scenarioSelectionOpen, ImGuiWindowFlags_AlwaysAutoResize);

      static int          n[2]         = {10, 10};
      static ScenarioType scenarioType = ScenarioType::Test; // TODO: Do better

      ImGui::InputInt2("Grid Dimensions", n, 0);

      if (ImGui::BeginCombo("Scenario", scenarioTypeToString(scenarioType).c_str())) {
        for (int i = 0; i < (int)ScenarioType::Count; i++) {
          ScenarioType type       = (ScenarioType)i;
          bool         isSelected = scenarioType == type;
          if (ImGui::Selectable(scenarioTypeToString(type).c_str(), isSelected)) {
            scenarioType = type;
          }
          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      if (ImGui::Button("Load Scenario")) {
        loadBlock(scenarioType, n[0], n[1]);
        m_simulationTime      = 0.0;
        scenarioSelectionOpen = false;
      }
      ImGui::End();
    }
  }

  void SweApp::update() {
    bgfx::dbgTextClear();
    bgfx::setDebug(m_debugFlags);

    bgfx::touch(0);

    bgfx::setUniform(u_color, m_color);
    bgfx::setUniform(u_util, m_util);

    int   nx      = m_block->getNx();
    int   ny      = m_block->getNy();
    float dx      = (float)m_block->getDx();
    float dy      = (float)m_block->getDy();
    float originX = (float)m_block->getOffsetX();
    float originY = (float)m_block->getOffsetY();

    { // TODO: Try out perspective projection and 3d camera
      float left   = originX;
      float right  = originX + dx * nx;
      float bottom = originY;
      float top    = originY + dy * ny;

      float aspect = (float)m_windowWidth / (float)m_windowHeight; // TODO: Ensure non-zero
      if (aspect > 1.0f) {
        left *= aspect;
        right *= aspect;
      } else {
        bottom /= aspect;
        top /= aspect;
      }

      float proj[16];
      bx::mtxOrtho(proj, left, right, bottom, top, m_cameraClipping[0], m_cameraClipping[1], 0, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Left);
      bgfx::setViewTransform(0, nullptr, proj);
    }

    { // TODO: Only do all this if old code would've written to file
      bgfx::TransientVertexBuffer tvb;
      bgfx::TransientIndexBuffer  tib;
      bgfx::allocTransientBuffers(&tvb, CellVertex::layout, nx * ny, &tib, (nx - 1) * (ny - 1) * 6);

      Float2D<CellVertex>      vertices(ny, nx, (CellVertex*)tvb.data);
      const Float2D<RealType>& heights = m_block->getWaterHeight();

      for (int j = 1; j <= ny; j++) {
        for (int i = 1; i <= nx; i++) {
          float x                = originX + (i - 0.5f) * dx;
          float y                = originY + (j - 0.5f) * dy;
          float h                = (float)heights[j][i];
          vertices[j - 1][i - 1] = {x, y, h};
        }
      }

      uint16_t*   indices      = (uint16_t*)tib.data;
      CellVertex* verticesBase = vertices.getData();
      int         c            = 0;

      for (int j = 0; j < ny - 1; j++) {
        for (int i = 0; i < nx - 1; i++) {
          int topLeft     = &vertices[j][i] - verticesBase;
          int topRight    = topLeft + 1;
          int bottomLeft  = &vertices[j + 1][i] - verticesBase;
          int bottomRight = bottomLeft + 1;
          // Draw clockwise because y is flipped from projection and CW-culling is default
          indices[c++] = topLeft;
          indices[c++] = topRight;
          indices[c++] = bottomLeft;
          indices[c++] = bottomLeft;
          indices[c++] = topRight;
          indices[c++] = bottomRight;
        }
      }

      bgfx::setIndexBuffer(&tib);
      bgfx::setVertexBuffer(0, &tvb);
      bgfx::submit(0, m_program);
    }

    bgfx::setState(BGFX_STATE_DEFAULT);
    bgfx::submit(0, m_program);

    bgfx::frame();
  }

  static std::string scenarioTypeToString(ScenarioType type) {
    switch (type) {
    case ScenarioType::Tsunami:
      return "Tsunami";
    case ScenarioType::ArtificialTsunami:
      return "Artificial Tsunami";
    case ScenarioType::Test:
      return "Test";
    default:
      assert(false);
    }
    return {};
  }

} // namespace App

Core::Application* Core::createApplication(int, char**) { return new App::SweApp(); }
