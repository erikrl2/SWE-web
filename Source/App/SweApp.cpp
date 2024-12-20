#include "SweApp.hpp"

#include <algorithm>
#include <bgfx/embedded_shader.h>
#include <bx/math.h>
#include <imgui.h>
#include <iostream>

#include "Blocks/DimensionalSplitting.hpp"
#include "Scenarios/ArtificialTsunamiScenario.hpp"
#include "Scenarios/TestScenario.hpp"
#include "Scenarios/TsunamiScenario.hpp"
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
    glfwSetDropCallback(m_window, dropFileCallback);

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
    if (m_scenario) {
      delete m_scenario;
    }
    bgfx::destroy(m_program);
    bgfx::destroy(u_color);
    bgfx::destroy(u_util);
  }

  static std::string scenarioTypeToString(ScenarioType type);

  void SweApp::loadBlock() {
    if (m_scenario) {
      delete m_scenario;
    }

    switch (m_scenarioType) {
    case ScenarioType::Test:
      m_scenario = new Scenarios::TestScenario(m_dimensions[0]);
      break;
    case ScenarioType::ArtificialTsunami:
      // TODO: Implement BoundaryType selection
      m_scenario = new Scenarios::ArtificialTsunamiScenario(0, BoundaryType::Wall);
      break;
    case ScenarioType::Tsunami: {
#ifdef __EMSCRIPTEN__
      std::cout << "Tsunami scenario not implemented in Emscripten" << std::endl;
#else
      const auto* s = new Scenarios::TsunamiScenario(m_bathymetryFile, m_displacementFile, 0, BoundaryType::Outflow, m_dimensions[0], m_dimensions[1]);
      if (s->success()) {
        m_scenario = s;
        break;
      } else {
        std::cerr << "Failed to load Tsunami scenario" << std::endl;
        delete s;
      }
#endif
      [[fallthrough]];
    }
    case ScenarioType::None:
      m_scenarioType  = ScenarioType::None;
      m_dimensions[0] = 0;
      m_dimensions[1] = 0;
      m_block         = nullptr;
      m_scenario      = nullptr;
      return;
    default:
      assert(false); // Shouldn't happen
    }

    RealType left   = m_scenario->getBoundaryPos(BoundaryEdge::Left);
    RealType right  = m_scenario->getBoundaryPos(BoundaryEdge::Right);
    RealType bottom = m_scenario->getBoundaryPos(BoundaryEdge::Bottom);
    RealType top    = m_scenario->getBoundaryPos(BoundaryEdge::Top);

    int      nx = m_dimensions[0];
    int      ny = m_dimensions[1];
    RealType dx = (right - left) / m_dimensions[0];
    RealType dy = (top - bottom) / m_dimensions[1];

    std::cout << "Loading block with scenario: " << scenarioTypeToString(m_scenarioType) << std::endl;
    std::cout << "  nx: " << nx << ", ny: " << ny << std::endl;
    std::cout << "  dx: " << dx << ", dy: " << dy << std::endl;
    std::cout << "  Left: " << left << ", Right: " << right << ", Bottom: " << bottom << ", Top: " << top << std::endl;

    if (m_block) {
      delete m_block;
    }
    m_block = new Blocks::DimensionalSplittingBlock(nx, ny, dx, dy);
    m_block->initialiseScenario(left, bottom, *m_scenario);
    m_block->setGhostLayer();

    const RealType* h = m_block->getWaterHeight().getData(); // TODO: Try h, hu, hv, b, h+b

    auto [min, max]               = std::minmax_element(h, h + (m_block->getNx() + 2) * (m_block->getNy() + 2));
    m_util[UtilIndex::hMin]       = (float)*min - 0.1f;
    m_util[UtilIndex::hMax]       = (float)*max + 0.1f;
    m_util[UtilIndex::heightExag] = 1.0f;

    m_cameraClipping[0] = (float)*min - 10.0f;
    m_cameraClipping[1] = (float)*max + 10.0f;
  }

  void SweApp::update() {
    bgfx::touch(0); // So window is cleared if nothing is submitted

    updateTransform();

    if (m_playing) {
      m_block->setGhostLayer();

#if 1 // First option needs more computation but is more stable for non-square cells
      m_block->computeMaxTimeStep();
      RealType dt = m_block->getMaxTimeStep();
      m_block->simulateTimeStep(dt);
#else
      m_block->computeNumericalFluxes();
      RealType dt = m_block->getMaxTimeStep();
      m_block->updateUnknowns(dt);
#endif

      m_simulationTime += dt;
    }
    submitMesh();

    bgfx::frame();
  }

  void SweApp::updateTransform() {
    if (!m_block)
      return;
    if (m_windowWidth == 0 || m_windowHeight == 0)
      return;

    // TODO: Try out perspective projection and 3d camera

    float left   = (float)m_block->getOffsetX();
    float right  = left + (float)m_block->getDx() * m_dimensions[0];
    float bottom = (float)m_block->getOffsetY();
    float top    = bottom + (float)m_block->getDy() * m_dimensions[1];

    // TODO: Fix issue with aspect ratio
    // The domains aspect ration needs to be combined with aspect ratio of the window

    // int domainWidth  = m_dimensions[0] * m_block->getDx();
    // int domainHeight = m_dimensions[1] * m_block->getDy();

    // float aspectDomain = (float)domainWidth / (float)domainHeight;
    // float aspectWindow = (float)m_windowWidth / (float)m_windowHeight;

    // if (aspect > 1.0f) {
    //   left *= aspect;
    //   right *= aspect;
    // } else {
    //   bottom /= aspect;
    //   top /= aspect;
    // }

    float proj[16];
    bx::mtxOrtho(proj, left, right, bottom, top, m_cameraClipping[0], m_cameraClipping[1], 0, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Left);
    bgfx::setViewTransform(0, nullptr, proj);
  }

  void SweApp::submitMesh() {
    if (!m_block)
      return;

    int   nx      = m_dimensions[0];
    int   ny      = m_dimensions[1];
    float dx      = (float)m_block->getDx();
    float dy      = (float)m_block->getDy();
    float originX = (float)m_block->getOffsetX();
    float originY = (float)m_block->getOffsetY();

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    bgfx::allocTransientBuffers(&tvb, CellVertex::layout, nx * ny, &tib, (nx - 1) * (ny - 1) * 6);

    Float2D<CellVertex> vertices(ny, nx, (CellVertex*)tvb.data);

    // TODO: User selection between h, hu, hv, b, h+b
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
    ImGui::Text("Current Scenario: %s (%dx%d)", scenarioTypeToString(m_scenarioType).c_str(), m_dimensions[0], m_dimensions[1]);
    if (ImGui::Button("Reset")) {
      if (m_block) {
        m_block->initialiseScenario(m_block->getOffsetX(), m_block->getOffsetY(), *m_scenario);
        m_simulationTime = 0.0;
        m_playing        = false;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button(!m_playing ? "Start" : "Stop")) {
      if (m_block) {
        m_playing = !m_playing;
      }
    }
    ImGui::SameLine();
    ImGui::Text("Simulation time: %.2f seconds", m_simulationTime);

    ImGui::Separator();

    ImGui::Text("Visualization:");
    ImGui::ColorEdit4("Grid Color", m_color, ImGuiColorEditFlags_NoAlpha); // TODO: Alpha
    if (ImGui::DragFloat2("H Min/Max", &m_util[UtilIndex::hMin], 0.01f)) {
      m_util[UtilIndex::hMax] = std::max(m_util[UtilIndex::hMin], m_util[UtilIndex::hMax]);
    }
    if (ImGui::DragFloat2("Near/Far Clip", m_cameraClipping, 0.1f)) {
      m_cameraClipping[0] = std::min(m_cameraClipping[0], m_cameraClipping[1]);
    }
    ImGui::DragFloat("Height Exaggeration", &m_util[UtilIndex::heightExag], 0.01f, 0.0f, 100.0f);

    static float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (ImGui::ColorEdit3("Background Color", clearColor)) {
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

      ImGui::Text("Hint: Tsunami Scenario is Desktop only");

      static int          n[2]         = {2, 2};
      static ScenarioType scenarioType = m_scenarioType;

      if (ImGui::InputInt2("Grid Dimensions (1-100)", n)) {
        n[0] = std::clamp(n[0], 2, 100);
        n[1] = std::clamp(n[1], 2, 100);
      }
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

      if (scenarioType == ScenarioType::Tsunami) {
        ImGui::Text("Tsunami scenario requires bathymetry and displacement files.");
        ImGui::Text("Enter paths or drag/drop files to the window.");

        ImGui::InputText("Bathymetry File", m_bathymetryFile, sizeof(m_bathymetryFile));
        ImGui::InputText("Displacement File", m_displacementFile, sizeof(m_displacementFile));
      }

      if (ImGui::Button("Load Scenario") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        m_scenarioType        = scenarioType;
        m_dimensions[0]       = n[0];
        m_dimensions[1]       = n[1];
        m_simulationTime      = 0.0;
        m_playing             = false;
        scenarioSelectionOpen = false;

        loadBlock();
      }
      ImGui::End();
    }
  }

  void SweApp::dropFileCallback(GLFWwindow*, int count, const char** paths) {
    SweApp& app = dynamic_cast<SweApp&>(*Core::Application::get());

    for (int i = 0; i < count; i++) {
      const char* path = paths[i];
      if (std::string_view(path).ends_with(".nc")) {
        if (std::string_view(path).find("bath") != std::string::npos) {
          strncpy(app.m_bathymetryFile, path, sizeof(app.m_bathymetryFile));
        } else if (std::string_view(path).find("displ") != std::string::npos) {
          strncpy(app.m_displacementFile, path, sizeof(app.m_displacementFile));
        }
      }
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
