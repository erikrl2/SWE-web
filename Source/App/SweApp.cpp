#include "SweApp.hpp"

#include <algorithm>
#include <bgfx/embedded_shader.h>
#include <bx/math.h>
#include <imgui.h>
#include <iostream>

#ifdef ENABLE_OPENMP
#include <omp.h>
#endif

#include "Blocks/DimensionalSplitting.hpp"
#include "Scenarios/ArtificialTsunamiScenario.hpp"
#include "Scenarios/TestScenario.hpp"
#include "Scenarios/TsunamiScenario.hpp"
#include "swe/fs_swe.bin.h"
#include "swe/vs_swe.bin.h"

int g_useOpenMP = 0;

namespace App {

  static const bgfx::EmbeddedShader shaders[] = {
    BGFX_EMBEDDED_SHADER(vs_swe),
    BGFX_EMBEDDED_SHADER(fs_swe),

    BGFX_EMBEDDED_SHADER_END()
  };

  bgfx::VertexLayout CellVertex::layout;

  void CellVertex::init() { layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end(); };

  static std::string scenarioTypeToString(ScenarioType type);
  static std::string viewTypeToString(ViewType type);
  static uint32_t    colorToInt(float* color4);

  // Has to match order in shader
  enum UtilIndex { Min, Max, ValueScale };

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

    bgfx::setDebug(m_debugFlags);
    bgfx::reset(m_windowWidth, m_windowHeight, m_resetFlags);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, colorToInt(m_clearColor));
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

  void SweApp::update(float dt) {
    bgfx::touch(0); // clears window if nothing is submitted

    updateTransform();

    if (m_playing) {
      m_block->setGhostLayer();

      RealType scaleFactor = RealType(std::min(dt * m_timeScale, 1.0f));

#if 1 // First option needs more computation but is more stable for grids with non-square cells
      m_block->computeMaxTimeStep();
      RealType maxTimeStep = m_block->getMaxTimeStep();
      maxTimeStep *= scaleFactor;
      m_block->simulateTimeStep(maxTimeStep);
#else
      m_block->computeNumericalFluxes();
      RealType maxTimeStep = m_block->getMaxTimeStep();
      maxTimeStep *= scaleFactor;
      m_block->updateUnknowns(maxTimeStep);
#endif

      m_simulationTime += maxTimeStep;
    }

    submitMesh();

    bgfx::frame();
  }

  void SweApp::updateImGui(float dt) {
    static bool scenarioSelectionOpen = false;

    // ImGui::SetNextWindowPos(ImVec2(20, m_windowHeight - 320), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);

    ImGui::SeparatorText("Simulation");

    if (ImGui::Button("Select Scenario") || ImGui::IsKeyPressed(ImGuiKey_S)) {
      scenarioSelectionOpen = true;
    }
    ImGui::SameLine();
    ImGui::Text("%s (%dx%d)", scenarioTypeToString(m_scenarioType).c_str(), m_dimensions[0], m_dimensions[1]);
    if (ImGui::Button("Reset") || ImGui::IsKeyPressed(ImGuiKey_R)) {
      if (m_block) {
        m_block->initialiseScenario(m_block->getOffsetX(), m_block->getOffsetY(), *m_scenario);
        m_simulationTime = 0.0;
        m_playing        = false;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button(!m_playing ? "Start" : "Stop") || ImGui::IsKeyPressed(ImGuiKey_Space)) {
      if (m_block) {
        m_playing = !m_playing;
      }
    }
    ImGui::SameLine();
    ImGui::Text("Time: %.1f s", m_simulationTime);
    ImGui::DragFloat("Time Scale", &m_timeScale, 0.1f, 0.0f, 1.0f / dt);

    if (ImGui::BeginCombo("View Type", viewTypeToString(m_viewType).c_str())) {
      for (int i = 0; i < (int)ViewType::Count; i++) {
        ViewType type = (ViewType)i;
        if (ImGui::Selectable(viewTypeToString(type).c_str(), m_viewType == type)) {
          m_viewType = type;
          rescaleToDataRange();
        }
      }
      ImGui::EndCombo();
    }

    ImGui::SeparatorText("Visualization");

    ImGui::ColorEdit3("Grid Color", m_color, ImGuiColorEditFlags_NoAlpha);
    if (ImGui::DragFloat2("Data Range", &m_util[UtilIndex::Min], 0.01f)) {
      m_util[UtilIndex::Max] = std::max(m_util[UtilIndex::Min], m_util[UtilIndex::Max]);
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto Scale")) {
      rescaleToDataRange();
    }
    if (ImGui::DragFloat2("Near/Far Clip", m_cameraClipping, 0.1f)) {
      m_cameraClipping[0] = std::min(m_cameraClipping[0], m_cameraClipping[1]);
    }
    ImGui::DragFloat("Value Scale", &m_util[UtilIndex::ValueScale], 0.01f, 0.0f, 100.0f);

    if (ImGui::ColorEdit3("Background Color", m_clearColor)) {
      bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, colorToInt(m_clearColor));
    }

    ImGui::SeparatorText("Debug Options");

    static bool stats = m_debugFlags & BGFX_DEBUG_STATS;
    if (ImGui::Checkbox("Stats", &stats)) {
      m_debugFlags ^= BGFX_DEBUG_STATS;
      bgfx::setDebug(m_debugFlags);
    }
    ImGui::SameLine();
    static bool wireframe = m_stateFlags & BGFX_STATE_PT_LINES;
    if (ImGui::Checkbox("Lines", &wireframe)) {
      m_stateFlags ^= BGFX_STATE_PT_LINES;
    }
    ImGui::SameLine();
    static bool points = m_stateFlags & BGFX_STATE_PT_POINTS;
    if (ImGui::Checkbox("Points", &points)) {
      m_stateFlags ^= BGFX_STATE_PT_POINTS;
    }

    ImGui::SeparatorText("Performance");

    static bool vsync = m_resetFlags & BGFX_RESET_VSYNC;
    if (ImGui::Checkbox("VSync", &vsync)) {
      m_resetFlags ^= BGFX_RESET_VSYNC;
      bgfx::reset(m_windowWidth, m_windowHeight, m_resetFlags);
    }
    ImGui::SameLine();
#ifdef ENABLE_OPENMP
    ImGui::Checkbox("Use OpenMP", (bool*)&g_useOpenMP);
    if (g_useOpenMP) {
      static int maxThreads = omp_get_max_threads();
      static int ompThreadCount = maxThreads;
      ImGui::SameLine();
      ImGui::SetNextItemWidth(50.0f);
      if (ImGui::SliderInt("Threads", &ompThreadCount, 1, maxThreads)) {
        omp_set_num_threads(ompThreadCount);
      }
    }
#endif
    ImGui::SameLine();
    ImGui::TextDisabled("FPS: %.0f", 1.0f / dt);

    ImGui::SameLine(ImGui::GetWindowSize().x - 30);
    ImGui::TextDisabled("(?)");
    const char* helpText = "Press 'S' to open scenario selection.\n"
                           "Press 'Enter' to load scenario.\n"
                           "Press 'Esc' to close scenario selection.\n"
                           "Press 'Space' to start/stop simulation.\n"
                           "Press 'R' to reset simulation.\n"
                           "Drag and drop NetCDF files to set paths for Tsunami scenario.";
    ImGui::SetItemTooltip("%s", helpText);

    ImGui::End(); // Controls

    if (scenarioSelectionOpen) {
      // ImGui::SetNextWindowPos(ImVec2(m_windowWidth / 2 - 200, m_windowHeight / 2 - 50), ImGuiCond_FirstUseEver);
      ImGui::Begin("Scenario Selection", &scenarioSelectionOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

      if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        scenarioSelectionOpen = false;
      }

      static int          n[2]         = {100, 100};
      static ScenarioType scenarioType = ScenarioType::ArtificialTsunami;

      if (ImGui::BeginCombo("Scenario", scenarioTypeToString(scenarioType).c_str())) {
        for (int i = 0; i < (int)ScenarioType::Count; i++) {
          ScenarioType type = (ScenarioType)i;
          if (ImGui::Selectable(scenarioTypeToString(type).c_str(), scenarioType == type)) {
            scenarioType = type;
          }
        }
        ImGui::EndCombo();
      }
      if (ImGui::InputInt2("Grid Dimensions", n)) {
        n[0] = std::clamp(n[0], 2, 295);
        n[1] = std::clamp(n[1], 2, 295);
      }

#ifndef __EMSCRIPTEN__
      if (scenarioType == ScenarioType::Tsunami) {
        ImGui::Text("Requires bathymetry and displacement NetCDF files.");
        ImGui::Text("Enter paths or drag/drop files to the window.");
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        ImGui::SetItemTooltip("File name should contain 'bath' or 'displ' to be recognized.");

        ImGui::InputText("Bathymetry File", m_bathymetryFile, sizeof(m_bathymetryFile));
        ImGui::InputText("Displacement File", m_displacementFile, sizeof(m_displacementFile));
      }
#endif

      if (ImGui::Button("Load Scenario") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
        m_scenarioType        = scenarioType;
        m_dimensions[0]       = n[0];
        m_dimensions[1]       = n[1];
        m_simulationTime      = 0.0;
        m_playing             = false;
        scenarioSelectionOpen = false;

        loadBlock();
      }

      ImGui::End(); // Scenario Selection
    }
  }

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
#ifndef __EMSCRIPTEN__
    case ScenarioType::Tsunami: {
      const auto* s = new Scenarios::TsunamiScenario(m_bathymetryFile, m_displacementFile, 0, BoundaryType::Outflow, m_dimensions[0], m_dimensions[1]);
      if (s->success()) {
        m_scenario = s;
        break;
      }
      std::cerr << "Failed to load Tsunami scenario" << std::endl;
      delete s;
      [[fallthrough]];
    }
#endif
    case ScenarioType::None:
      m_scenarioType  = ScenarioType::None;
      m_dimensions[0] = 0;
      m_dimensions[1] = 0;
      m_block         = nullptr;
      m_scenario      = nullptr;
      return;
    default:
      assert(false);
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

    m_util[UtilIndex::ValueScale] = 1.0f;
    rescaleToDataRange();
  }

  void SweApp::rescaleToDataRange() {
    if (!m_block)
      return;

    const RealType* values = nullptr;

    if (m_viewType == ViewType::H) {
      values = m_block->getWaterHeight().getData();
    } else if (m_viewType == ViewType::Hu) {
      values = m_block->getDischargeHu().getData();
    } else if (m_viewType == ViewType::Hv) {
      values = m_block->getDischargeHv().getData();
    } else if (m_viewType == ViewType::B) {
      values = m_block->getBathymetry().getData();
    } else if (m_viewType == ViewType::HPlusB) {
      const int size  = (m_dimensions[0] + 2) * (m_dimensions[1] + 2);
      RealType* hCopy = new RealType[size];
      memcpy(hCopy, m_block->getWaterHeight().getData(), size * sizeof(RealType));
      const RealType* b = m_block->getBathymetry().getData();
      for (int i = 0; i < size; i++) {
        hCopy[i] += b[i];
      }
      values = hCopy;
    }

    auto [min, max]        = std::minmax_element(values, values + (m_dimensions[0] + 2) * (m_dimensions[1] + 2));
    m_util[UtilIndex::Min] = (float)*min - 0.01f;
    m_util[UtilIndex::Max] = (float)*max + 0.01f;
    m_cameraClipping[0]    = (float)*min * m_util[UtilIndex::ValueScale] - 10.0f;
    m_cameraClipping[1]    = (float)*max * m_util[UtilIndex::ValueScale] + 10.0f;

    if (m_viewType == ViewType::HPlusB)
      delete values;
  }

  void SweApp::updateTransform() {
    if (!m_block)
      return;
    if (m_windowWidth == 0 || m_windowHeight == 0)
      return;

    // TODO: Option to switch to perspective

    float domainWidth  = m_dimensions[0] * m_block->getDx();
    float domainHeight = m_dimensions[1] * m_block->getDy();

    float left   = (float)m_block->getOffsetX();
    float right  = left + domainWidth;
    float bottom = (float)m_block->getOffsetY();
    float top    = bottom + domainHeight;

    float aspect = (float)m_windowWidth / (float)m_windowHeight;

    if (aspect > domainWidth / domainHeight) {
      float width = domainHeight * aspect;
      left += (domainWidth - width) / 2.0f;
      right = left + width;
    } else {
      float height = domainWidth / aspect;
      bottom += (domainHeight - height) / 2.0f;
      top = bottom + height;
    }

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
    bool success = bgfx::allocTransientBuffers(&tvb, CellVertex::layout, nx * ny, &tib, (nx - 1) * (ny - 1) * 6, true);
    if (!success) {
      std::cout << "Allocation failed. Grid too large." << std::endl;
      return;
    }

    Float2D<CellVertex> vertices(ny, nx, (CellVertex*)tvb.data);

    if (m_viewType == ViewType::HPlusB) {
      const auto& h = m_block->getWaterHeight();
      const auto& b = m_block->getBathymetry();
// #ifdef ENABLE_OPENMP
// #pragma omp parallel for schedule(static)
// #endif
      for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
          vertices[j][i] = {originX + (i + 0.5f) * dx, originY + (j + 0.5f) * dy, (float)(h[j + 1][i + 1] + b[j + 1][i + 1])};
        }
      }
    } else {
      const Float2D<RealType>* values = nullptr;
      if (m_viewType == ViewType::H) {
        values = &m_block->getWaterHeight();
      } else if (m_viewType == ViewType::Hu) {
        values = &m_block->getDischargeHu();
      } else if (m_viewType == ViewType::Hv) {
        values = &m_block->getDischargeHv();
      } else if (m_viewType == ViewType::B) {
        values = &m_block->getBathymetry();
      }
// #ifdef ENABLE_OPENMP
// #pragma omp parallel for schedule(static)
// #endif
      for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
          vertices[j][i] = {originX + (i + 0.5f) * dx, originY + (j + 0.5f) * dy, (float)(*values)[j + 1][i + 1]};
        }
      }
    }

    uint32_t* indices = (uint32_t*)tib.data;

    const CellVertex* verticesBase = vertices.getData();

    int c = 0;

    // TODO: Adjust loop so c is calculated and not incremented
// #ifdef ENABLE_OPENMP
// #pragma omp parallel for schedule(static)
// #endif
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

    bgfx::setState(m_stateFlags);
    bgfx::submit(0, m_program);
  }

  void SweApp::dropFileCallback(GLFWwindow*, int count, const char** paths) {
#ifndef __EMSCRIPTEN__
    SweApp& app = dynamic_cast<SweApp&>(*Core::Application::get());
    for (int i = 0; i < count; i++) {
      const char* path = paths[i];
      if (std::string_view(path).ends_with(".nc")) {
        auto rfind = std::string_view(path).rfind('/');
        if (rfind == std::string::npos) {
          rfind = std::string_view(path).rfind('\\'); // Windows
        }
        if (rfind != std::string::npos) {
          path += rfind + 1;
        }
        if (std::string_view(path).find("bath") != std::string::npos) {
          strncpy(app.m_bathymetryFile, paths[i], sizeof(app.m_bathymetryFile));
        } else if (std::string_view(path).find("displ") != std::string::npos) {
          strncpy(app.m_displacementFile, paths[i], sizeof(app.m_displacementFile));
        }
      }
    }
#endif
  }

  // Local helper functions

  static std::string scenarioTypeToString(ScenarioType type) {
    switch (type) {
#ifndef __EMSCRIPTEN__
    case ScenarioType::Tsunami:
      return "Tsunami";
#endif
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

  static std::string viewTypeToString(ViewType type) {
    switch (type) {
    case ViewType::H:
      return "Water Height";
    case ViewType::Hu:
      return "Water Momentum X";
    case ViewType::Hv:
      return "Water Momentum Y";
    case ViewType::B:
      return "Bathymetry";
    case ViewType::HPlusB:
      return "Water Height + Bathymetry";
    default:
      assert(false);
    }
    return {};
  }

  static uint32_t colorToInt(float* color4) {
    uint32_t color = 0;
    for (int i = 0; i < 4; i++) {
      color |= (uint32_t)(color4[i] * 255) << ((3 - i) * 8);
    }
    return color;
  }

} // namespace App

Core::Application* Core::createApplication(int, char**) { return new App::SweApp(); }
