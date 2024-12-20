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

  // Has to match order in shader
  enum UtilIndex { hMin, hMax, heightExag };

  SweApp::SweApp():
    Core::Application("Swe", 1280, 720) {
    bgfx::RendererType::Enum type           = bgfx::getRendererType();
    bgfx::ShaderHandle       vertexShader   = bgfx::createEmbeddedShader(shaders, type, "vs_swe");
    bgfx::ShaderHandle       fragmentShader = bgfx::createEmbeddedShader(shaders, type, "fs_swe");
    m_program                               = bgfx::createProgram(vertexShader, fragmentShader, true);

    CellVertex::init();

    u_color = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
    u_util  = bgfx::createUniform("u_util", bgfx::UniformType::Vec4);
  }

  SweApp::~SweApp() {
    if (m_block) {
      delete m_block;
    }
    bgfx::destroy(m_program);
    bgfx::destroy(u_color);
    bgfx::destroy(u_util);
  }

  static std::string scenarioTypeToString(ScenarioType type);

  void SweApp::loadBlock() {
    Scenarios::Scenario* scenario = nullptr;
    switch (m_currentScenario) {
    case ScenarioType::Test:
      scenario = new Scenarios::TestScenario(m_currentDimensions[0]);
      break;
    case ScenarioType::ArtificialTsunami:
      // TODO: Implement maxSimulationTime and boundaryType selection
      scenario = new Scenarios::ArtificialTsunamiScenario(100.0, BoundaryType::Wall);
      break;
    case ScenarioType::Tsunami:
    // TODO
    default:
      m_currentScenario      = ScenarioType::None;
      m_currentDimensions[0] = 0;
      m_currentDimensions[1] = 0;
      m_block                = nullptr;
      return;
    }

    RealType left   = scenario->getBoundaryPos(BoundaryEdge::Left);
    RealType right  = scenario->getBoundaryPos(BoundaryEdge::Right);
    RealType bottom = scenario->getBoundaryPos(BoundaryEdge::Bottom);
    RealType top    = scenario->getBoundaryPos(BoundaryEdge::Top);

    int      nx = m_currentDimensions[0];
    int      ny = m_currentDimensions[1];
    RealType dx = (right - left) / m_currentDimensions[0];
    RealType dy = (top - bottom) / m_currentDimensions[1];

    std::cout << "Loading block with scenario: " << scenarioTypeToString(m_currentScenario) << std::endl;
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
    const RealType* h             = m_block->getWaterHeight().getData();
    auto [min, max]               = std::minmax_element(h, h + (m_block->getNx() + 2) * (m_block->getNy() + 2));
    m_util[UtilIndex::hMin]       = (float)*min;
    m_util[UtilIndex::hMax]       = (float)*max;
    m_util[UtilIndex::heightExag] = 1.0f;

    m_cameraClipping[0] = (float)*min - 1.0f;
    m_cameraClipping[1] = (float)*max + 1.0f;
  }

  void SweApp::update() {
    bgfx::touch(0); // So window is cleared if nothing is submitted

    updateTransform();

    // TODO: Run simulation

    submitHeightGrid();

    bgfx::frame();
  }

  void SweApp::updateTransform() {
    if (!m_block)
      return;
    if (m_windowWidth == 0 || m_windowHeight == 0)
      return;

    // TODO: Try out perspective projection and 3d camera

    float left   = (float)m_block->getOffsetX();
    float right  = left + (float)m_block->getDx() * m_block->getNx();
    float bottom = (float)m_block->getOffsetY();
    float top    = bottom + (float)m_block->getDy() * m_block->getNy();

    float aspect = (float)m_windowWidth / (float)m_windowHeight;

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

  void SweApp::submitHeightGrid() {
    if (!m_block)
      return;

    // TODO: Only do all this if old code would've written to file

    int   nx      = m_block->getNx();
    int   ny      = m_block->getNy();
    float dx      = (float)m_block->getDx();
    float dy      = (float)m_block->getDy();
    float originX = (float)m_block->getOffsetX();
    float originY = (float)m_block->getOffsetY();

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    bgfx::allocTransientBuffers(&tvb, CellVertex::layout, nx * ny, &tib, (nx - 1) * (ny - 1) * 6);

    Float2D<CellVertex> vertices(ny, nx, (CellVertex*)tvb.data);

    const auto& heights = m_block->getWaterHeight();
    // const auto& bathymetry = m_block->getBathymetry();

    for (int j = 1; j <= ny; j++) {
      for (int i = 1; i <= nx; i++) {
        float x = originX + (i - 0.5f) * dx;
        float y = originY + (j - 0.5f) * dy;
        float h = (float)heights[j][i];
        // float b = (float)bathymetry[j][i];

        vertices[j - 1][i - 1] = {x, y, h};
        // vertices[j - 1][i - 1] = {x, y, h + b};
      }
    }

    uint16_t*   indices      = (uint16_t*)tib.data;
    int         c            = 0;
    const auto* verticesBase = vertices.getData();

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

    bgfx::setUniform(u_color, m_color);
    bgfx::setUniform(u_util, m_util);

    bgfx::setState(BGFX_STATE_DEFAULT);
    bgfx::submit(0, m_program);
  }

  void SweApp::updateImGui() {
    static bool scenarioSelectionOpen = false;

    // ImGui::SetNextWindowPos(ImVec2(20, m_windowHeight - 320), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Simulation:");
    if (ImGui::Button("Select Scenario")) {
      scenarioSelectionOpen = true;
    }
    ImGui::SameLine();
    ImGui::Text("Current Scenario: %s (%dx%d)", scenarioTypeToString(m_currentScenario).c_str(), m_currentDimensions[0], m_currentDimensions[1]);
    if (ImGui::Button("Start Simulation")) {
      // TODO
    }
    ImGui::SameLine();
    ImGui::Text("Simulation time: %.2f", m_simulationTime);

    ImGui::Separator();

    ImGui::Text("Visualization:");
    ImGui::ColorEdit4("Grid Color", m_color, ImGuiColorEditFlags_NoAlpha); // TODO: Alpha
    ImGui::DragFloat2("H Min/Max", &m_util[UtilIndex::hMin], 0.1f);
    ImGui::DragFloat2("Near/Far Clip", &m_cameraClipping[0]);
    ImGui::DragFloat("Height Exaggeration", &m_util[UtilIndex::heightExag], 0.01f, 0.0f, 100.0f);

    static float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (ImGui::ColorEdit4("Background Color", clearColor)) {
      uint32_t color = 0;
      for (int i = 0; i < 4; i++) {
        color |= (uint32_t)(clearColor[i] * 255) << ((3 - i) * 8);
      }
      bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, color);
    }

    ImGui::Separator();

    ImGui::Text("Debug options (toggle):");
    static int debugFlags = BGFX_DEBUG_NONE;
    if (ImGui::Button("Show stats"))
      debugFlags ^= BGFX_DEBUG_STATS;
    ImGui::SameLine();
    if (ImGui::Button("Wireframe"))
      debugFlags ^= BGFX_DEBUG_WIREFRAME;
    bgfx::setDebug(debugFlags);

    ImGui::Separator();

    if (ImGui::Button("Exit")) {
      glfwSetWindowShouldClose(m_window, 1);
    }
    ImGui::End();

    if (scenarioSelectionOpen) {
      ImGui::SetNextWindowPos(ImVec2(m_windowWidth / 2 - 194, m_windowHeight / 2 - 50), ImGuiCond_FirstUseEver);
      ImGui::Begin("Scenario Selection", &scenarioSelectionOpen, ImGuiWindowFlags_AlwaysAutoResize);

      static int n[2] = {2, 2};
      if (ImGui::InputInt2("Grid Dimensions (1-100)", n)) {
        n[0] = std::clamp(n[0], 2, 100);
        n[1] = std::clamp(n[1], 2, 100);
      }

      static ScenarioType scenarioType = m_currentScenario;
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
        m_currentScenario      = scenarioType;
        m_currentDimensions[0] = n[0];
        m_currentDimensions[1] = n[1];
        m_simulationTime       = 0.0;
        scenarioSelectionOpen  = false;

        loadBlock();
      }
      ImGui::End();
    }
  }

  static std::string scenarioTypeToString(ScenarioType type) {
    switch (type) {
    case ScenarioType::Tsunami:
      return "Tsunami";
    case ScenarioType::ArtificialTsunami:
      return "Artificial Tsunami";
    case ScenarioType::Test:
      return "Test";
    case ScenarioType::None:
      return "None";
    default:
      assert(false);
    }
    return {};
  }

} // namespace App

Core::Application* Core::createApplication(int, char**) { return new App::SweApp(); }
