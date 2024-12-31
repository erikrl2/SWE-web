#include "SweApp.hpp"

#include <algorithm>
#include <bgfx/embedded_shader.h>
#include <bx/math.h>
#include <filesystem>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <limits>

#ifdef ENABLE_OPENMP
#include <omp.h>
#endif

#include "Blocks/DimensionalSplitting.hpp"
#include "Scenarios/ArtificialTsunamiScenario.hpp"
#include "Scenarios/TestScenario.hpp"
#include "Scenarios/TsunamiScenario.hpp"
#include "swe/fs_swe.bin.h"
#include "swe/vs_swe.bin.h"
#include "Utils.hpp"

int g_useOpenMP = 0;

namespace App {

  SweApp::SweApp():
    Core::Application("Swe", 1280, 720) {

    bgfx::RendererType::Enum type           = bgfx::getRendererType();
    bgfx::ShaderHandle       vertexShader   = bgfx::createEmbeddedShader(shaders, type, "vs_swe");
    bgfx::ShaderHandle       fragmentShader = bgfx::createEmbeddedShader(shaders, type, "fs_swe");
    m_program                               = bgfx::createProgram(vertexShader, fragmentShader, true);

    CellVertex::init();

    u_gridData    = bgfx::createUniform("u_gridData", bgfx::UniformType::Vec4);
    u_boundaryPos = bgfx::createUniform("u_boundaryPos", bgfx::UniformType::Vec4);
    u_util        = bgfx::createUniform("u_util", bgfx::UniformType::Vec4);
    u_color       = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
    u_heightMap   = bgfx::createUniform("u_heightMap", bgfx::UniformType::Sampler);

    bgfx::setDebug(m_debugFlags);
    bgfx::reset(m_windowSize.x, m_windowSize.y, m_resetFlags);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, colorToInt(m_clearColor));
  }

  SweApp::~SweApp() {
    if (m_scenario) {
      delete m_scenario;
      delete m_block;
      bgfx::destroy(m_vbh);
      bgfx::destroy(m_ibh);
      bgfx::destroy(m_heightMap);
    }

    bgfx::destroy(u_gridData);
    bgfx::destroy(u_boundaryPos);
    bgfx::destroy(u_util);
    bgfx::destroy(u_color);
    bgfx::destroy(u_heightMap);

    bgfx::destroy(m_program);
  }

  void SweApp::update(float dt) {
    bgfx::touch(0); // clears window if nothing is submitted

    updateCamera(dt);

    if (m_block && m_playing) {
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

      m_simulationTime += (float)maxTimeStep;

      if (m_endSimulationTime > 0.0 && m_simulationTime >= m_endSimulationTime) {
        m_playing = false;
      }
    }

    updateGrid();

    bgfx::frame();
  }

  void SweApp::updateImGui(float dt) {
    if (!m_showControls)
      return;

    // ImGui::SetNextWindowPos(ImVec2(20, m_windowHeight - 320), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);

    ImGui::SeparatorText("Simulation");

    if (ImGui::Button("Select Scenario")) {
      m_showScenarioSelection = true;
    }

    ImGui::SameLine();
    ImGui::Text("%s (%dx%d)", scenarioTypeToString(m_scenarioType).c_str(), m_dimensions.x, m_dimensions.y);
    if (ImGui::Button("Reset")) {
      resetScenario();
    }

    ImGui::SameLine();
    if (ImGui::Button(!m_playing ? "Start" : "Stop")) {
      m_playing = !m_playing;
    }

    ImGui::SameLine();
    ImGui::Text("Time: %.1f s", m_simulationTime);

    if (ImGui::BeginCombo("View Type", viewTypeToString(m_viewType).c_str())) {
      for (int i = 0; i < (int)ViewType::Count; i++) {
        ViewType type = (ViewType)i;
        if (ImGui::Selectable(viewTypeToString(type).c_str(), m_viewType == type)) {
          m_viewType = type;
          rescaleToDataRange();
          setupCamera();
        }
      }
      ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Boundary Type", boundaryTypeToString(m_boundaryType).c_str())) {
      for (int i = 0; i < (int)BoundaryType::Count; i++) {
        BoundaryType type = (BoundaryType)i;
        if (ImGui::Selectable(boundaryTypeToString(type).c_str(), m_boundaryType == type)) {
          m_boundaryType = type;
          setBlockBoundaryType(m_block, m_boundaryType);
        }
      }
      ImGui::EndCombo();
    }

    ImGui::DragFloat("Time Scale", &m_timeScale, 0.1f, 0.0f, 1.0f / dt);

    ImGui::DragFloat("End Simulation Time", &m_endSimulationTime, 1.0f, 0.0f, std::numeric_limits<float>::max());

    ImGui::SeparatorText("Visualization");

    if (ImGui::DragFloat2("Data Range", m_util, 0.01f)) {
      m_util.y = std::max(m_util.x, m_util.y);
    }

    ImGui::SameLine();
    if (ImGui::Button("Auto Scale")) {
      rescaleToDataRange();
    }

    ImGui::ColorEdit3("Grid Color", m_color, ImGuiColorEditFlags_NoAlpha);

    if (ImGui::DragFloat2("Near/Far Clip", m_cameraClipping, 0.1f)) {
      m_cameraClipping.x = std::max(0.1f, std::min(m_cameraClipping.x, m_cameraClipping.y - 0.1f));
      m_cameraClipping.y = std::max(m_cameraClipping.x + 0.1f, m_cameraClipping.y);
    }

    if (ImGui::DragFloat("Value Scale", &m_util.z, 0.01f, 0.0f, 100.0f)) {
      setupCamera();
    }

    if (ImGui::ColorEdit3("Background Color", m_clearColor)) {
      bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, colorToInt(m_clearColor));
    }

    ImGui::SeparatorText("Camera");

    if (ImGui::Checkbox("3D", &m_cameraIs3D)) {
      m_camera.setType(m_camera.getType() == Camera::Type::Orthographic ? Camera::Type::Perspective : Camera::Type::Orthographic);
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset Camera")) {
      m_camera.reset();
    }

    ImGui::SeparatorText("Debug Options");

    if (ImGui::Checkbox("Stats", &m_showStats)) {
      m_debugFlags ^= BGFX_DEBUG_STATS;
      bgfx::setDebug(m_debugFlags);
    }

    ImGui::SameLine();
    if (ImGui::Checkbox("Wireframe", &m_showLines)) {
      m_stateFlags ^= BGFX_STATE_PT_LINES;
    }

#ifndef __EMSCRIPTEN__
    ImGui::SeparatorText("Performance");

    static bool vsync = m_resetFlags & BGFX_RESET_VSYNC;
    if (ImGui::Checkbox("VSync", &vsync)) {
      m_resetFlags ^= BGFX_RESET_VSYNC;
      bgfx::reset(m_windowSize.x, m_windowSize.y, m_resetFlags);
    }

    ImGui::SameLine();
#ifdef ENABLE_OPENMP
    ImGui::Checkbox("Use OpenMP", (bool*)&g_useOpenMP);
    if (g_useOpenMP) {
      static const int maxThreads     = omp_get_max_threads();
      static int       ompThreadCount = maxThreads;
      ImGui::SameLine();
      ImGui::SetNextItemWidth(50.0f);
      if (ImGui::SliderInt("Threads", &ompThreadCount, 1, maxThreads)) {
        omp_set_num_threads(ompThreadCount);
      }
    }
#endif
#endif

    ImGui::SameLine();
    ImGui::TextDisabled("FPS: %.0f", 1.0f / dt);

    ImGui::SameLine(ImGui::GetWindowSize().x - 30);
    ImGui::TextDisabled("(?)");
    static const char* helpText = R"(Key Bindings:

  C         : hide control window
  S         : open scenario selection.
  Space     : start/stop simulation.
  R         : reset simulation.
  H/U/V/B/A : select view type
  O/W       : select boundary type
  Q         : auto rescale data range
  T         : switch camera type
  X         : reset camera
  L         : show lines
  I         : show stats
)";
    ImGui::SetItemTooltip("%s", helpText);

    ImGui::End(); // Controls

    if (m_showScenarioSelection) {
      // ImGui::SetNextWindowPos(ImVec2(m_windowWidth / 2 - 200, m_windowHeight / 2 - 50), ImGuiCond_FirstUseEver);
      ImGui::Begin("Scenario Selection", &m_showScenarioSelection, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

      if (ImGui::BeginCombo("Scenario", scenarioTypeToString(m_selectedScenarioType).c_str())) {
        for (int i = 0; i < (int)ScenarioType::Count; i++) {
          ScenarioType type = (ScenarioType)i;
          if (ImGui::Selectable(scenarioTypeToString(type).c_str(), m_selectedScenarioType == type)) {
            m_selectedScenarioType = type;
          }
        }
        ImGui::EndCombo();
      }

      if (ImGui::InputInt2("Grid Dimensions", m_selectedDimensions)) {
        m_selectedDimensions.x = std::clamp(m_selectedDimensions.x, 2, 2000);
        m_selectedDimensions.y = std::clamp(m_selectedDimensions.y, 2, 2000);
      }

#ifndef __EMSCRIPTEN__
      if (m_selectedScenarioType == ScenarioType::Tsunami) {
        ImGui::Text("Requires bathymetry and displacement NetCDF files.");
        ImGui::Text("Enter paths or drag/drop files to the window.");

        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        ImGui::SetItemTooltip("File name must contain 'bath' or 'displ' to be recognized.");

        ImGui::InputText("Bathymetry File", m_bathymetryFile, sizeof(m_bathymetryFile));
        ImGui::InputText("Displacement File", m_displacementFile, sizeof(m_displacementFile));
      }
#endif

      if (ImGui::Button("Load Scenario")) {
        loadScenario();
      }

      ImGui::End(); // Scenario Selection
    }
  }

  void SweApp::loadScenario() {
    m_scenarioType          = m_selectedScenarioType;
    m_dimensions            = m_selectedDimensions;
    m_simulationTime        = 0.0;
    m_playing               = false;
    m_showScenarioSelection = false;

    loadBlock();
  }

  void SweApp::resetScenario() {
    if (!m_block)
      return;

    m_block->initialiseScenario(m_block->getOffsetX(), m_block->getOffsetY(), *m_scenario);
    m_simulationTime = 0.0;
    m_playing        = false;

    setBlockBoundaryType(m_block, m_boundaryType);
  }

  void SweApp::loadBlock() {
    if (m_scenario) {
      delete m_scenario;
      delete m_block;
      bgfx::destroy(m_vbh);
      bgfx::destroy(m_ibh);
      bgfx::destroy(m_heightMap);
    }

    switch (m_scenarioType) {
    case ScenarioType::Test:
      m_scenario = new Scenarios::TestScenario(m_boundaryType, m_dimensions.x);
      break;
    case ScenarioType::ArtificialTsunami:
      m_scenario = new Scenarios::ArtificialTsunamiScenario(m_boundaryType);
      break;
#ifndef __EMSCRIPTEN__
    case ScenarioType::Tsunami: {
      const auto* s = new Scenarios::TsunamiScenario(m_bathymetryFile, m_displacementFile, m_boundaryType, m_dimensions.x, m_dimensions.y);
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
      m_scenarioType = ScenarioType::None;
      m_dimensions   = {0, 0};
      m_block        = nullptr;
      m_scenario     = nullptr;
      return;
    default:
      assert(false);
    }

    RealType left   = m_scenario->getBoundaryPos(BoundaryEdge::Left);
    RealType right  = m_scenario->getBoundaryPos(BoundaryEdge::Right);
    RealType bottom = m_scenario->getBoundaryPos(BoundaryEdge::Bottom);
    RealType top    = m_scenario->getBoundaryPos(BoundaryEdge::Top);

    m_boundaryPos = {(float)left, (float)right, (float)bottom, (float)top};

    int      nx = m_dimensions.x;
    int      ny = m_dimensions.y;
    RealType dx = (right - left) / RealType(nx);
    RealType dy = (top - bottom) / RealType(ny);

    m_gridData = {(float)nx, (float)ny, (float)dx, (float)dy};

    std::cout << "Loading block with scenario: " << scenarioTypeToString(m_scenarioType) << std::endl;
    std::cout << "  nx: " << nx << ", ny: " << ny << std::endl;
    std::cout << "  dx: " << dx << ", dy: " << dy << std::endl;
    std::cout << "  Left: " << left << ", Right: " << right << ", Bottom: " << bottom << ", Top: " << top << std::endl;

    m_block = new Blocks::DimensionalSplittingBlock(nx, ny, dx, dy);
    m_block->initialiseScenario(left, bottom, *m_scenario);
    m_block->setGhostLayer();

    rescaleToDataRange();
    m_util.z = 1.0f;
    m_camera.reset();
    setupCamera();

    m_endSimulationTime = 0.0;

    m_vertices.resize(nx * ny);

    m_indices.clear();
    m_indices.reserve(2 * (nx + 1) * (ny - 1) - 2);
    for (int j = 0; j < ny - 1; j++) {
      for (int i = 0; i < nx; i++) {
        m_indices.push_back(j * nx + i);
        m_indices.push_back((j + 1) * nx + i);
      }
      if (j < ny - 2) {
        m_indices.push_back((j + 1) * nx + (nx - 1));
        m_indices.push_back((j + 1) * nx);
      }
    }

    m_vbh = bgfx::createVertexBuffer(bgfx::makeRef(m_vertices.data(), m_vertices.size() * sizeof(CellVertex)), CellVertex::layout);
    m_ibh = bgfx::createIndexBuffer(bgfx::makeRef(m_indices.data(), m_indices.size() * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);

    m_heightMap = bgfx::createTexture2D(nx, ny, false, 1, bgfx::TextureFormat::R32F, BGFX_TEXTURE_NONE);
  }

  void SweApp::rescaleToDataRange() {
    if (!m_block)
      return;

    const RealType* values = nullptr;

    if (m_viewType == ViewType::HPlusB) {
      const int size  = (m_dimensions.x + 2) * (m_dimensions.y + 2);
      RealType* hCopy = new RealType[size];
      memcpy(hCopy, m_block->getWaterHeight().getData(), size * sizeof(RealType));
      const RealType* b = m_block->getBathymetry().getData();
      for (int i = 0; i < size; i++) {
        hCopy[i] += b[i];
      }
      values = hCopy;
    } else {
      values = getBlockValues(m_block, m_viewType).getData();
    }

    auto [min, max] = std::minmax_element(values, values + (m_dimensions.x + 2) * (m_dimensions.y + 2));

    if (m_viewType == ViewType::HPlusB)
      delete values;

    m_util.x = (float)*min - 0.01f;
    m_util.y = (float)*max + 0.01f;
  }

  void SweApp::setupCamera() {
    float left   = m_boundaryPos.x;
    float right  = m_boundaryPos.y;
    float bottom = m_boundaryPos.z;
    float top    = m_boundaryPos.w;

    Vec3f center;
    center.x = (left + right) * 0.5f;
    center.y = (bottom + top) * 0.5f;
    center.z = (float)getScenarioValue(m_scenario, m_viewType, RealType(center.x), RealType(center.y)) * m_util.z;
    m_camera.setTargetCenter(center);

    float maxDim  = std::max(right - left, top - bottom);
    float maxDist = m_util.y - m_util.x;

    m_cameraClipping.x = std::min(m_gridData.z, m_gridData.w) * 0.1f;
    m_cameraClipping.y = maxDim * 10.0f + std::max(maxDim, maxDist);
    m_cameraClipping.x = std::max(0.1f, std::min(m_cameraClipping.x, m_cameraClipping.y - 0.1f));
    m_cameraClipping.y = std::max(m_cameraClipping.x + 0.1f, m_cameraClipping.y);
  }

  void SweApp::updateCamera(float dt) {
    if (!m_block)
      return;
    if (m_windowSize.x <= 0 || m_windowSize.y <= 0)
      return;

    m_camera.setMouseOverUI(ImGui::GetIO().WantCaptureMouse);

    m_camera.update(dt);
    m_camera.applyViewProjection();
  }

  void SweApp::updateGrid() {
    if (!m_block)
      return;

    int nx = m_dimensions.x;
    int ny = m_dimensions.y;

    if (m_viewType == ViewType::HPlusB) {
      const auto& h = m_block->getWaterHeight();
      const auto& b = m_block->getBathymetry();

      m_heightMapData.resize(nx * ny);
      for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
          m_heightMapData[j * nx + i] = (float)(h[j + 1][i + 1] + b[j + 1][i + 1]);
        }
      }
      bgfx::updateTexture2D(m_heightMap, 0, 0, 0, 0, nx, ny, bgfx::makeRef(m_heightMapData.data(), sizeof(float) * nx * ny));
    } else {
      const Float2D<RealType>& values = getBlockValues(m_block, m_viewType);

      m_heightMapData.resize(nx * ny);
      for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
          m_heightMapData[j * nx + i] = (float)values[j + 1][i + 1];
        }
      }
      bgfx::updateTexture2D(m_heightMap, 0, 0, 0, 0, nx, ny, bgfx::makeRef(m_heightMapData.data(), sizeof(float) * nx * ny));
    }

    bgfx::setIndexBuffer(m_ibh);
    bgfx::setVertexBuffer(0, m_vbh);

    bgfx::setTexture(0, u_heightMap, m_heightMap);

    bgfx::setUniform(u_gridData, m_gridData);
    bgfx::setUniform(u_boundaryPos, m_boundaryPos);
    bgfx::setUniform(u_util, m_util);
    bgfx::setUniform(u_color, m_color);

    bgfx::setState(m_stateFlags);
    bgfx::submit(0, m_program);
  }

  void SweApp::onResize(int, int) {}

  void SweApp::onKeyPressed(int key) {
    switch (key) {
    case GLFW_KEY_C:
      m_showControls = !m_showControls;
      break;
    case GLFW_KEY_S:
      m_showScenarioSelection = !m_showScenarioSelection;
      break;
    case GLFW_KEY_ENTER:
      if (m_showScenarioSelection)
        loadScenario();
      break;
    case GLFW_KEY_SPACE:
      m_playing = !m_playing;
      break;
    case GLFW_KEY_R:
      resetScenario();
      break;
    case GLFW_KEY_H:
      m_viewType = ViewType::H;
      rescaleToDataRange();
      setupCamera();
      break;
    case GLFW_KEY_U:
      m_viewType = ViewType::Hu;
      rescaleToDataRange();
      setupCamera();
      break;
    case GLFW_KEY_V:
      m_viewType = ViewType::Hv;
      rescaleToDataRange();
      setupCamera();
      break;
    case GLFW_KEY_B:
      m_viewType = ViewType::B;
      rescaleToDataRange();
      setupCamera();
      break;
    case GLFW_KEY_A:
      m_viewType = ViewType::HPlusB;
      rescaleToDataRange();
      setupCamera();
      break;
    case GLFW_KEY_O:
      m_boundaryType = BoundaryType::Outflow;
      setBlockBoundaryType(m_block, m_boundaryType);
      break;
    case GLFW_KEY_W:
      m_boundaryType = BoundaryType::Wall;
      setBlockBoundaryType(m_block, m_boundaryType);
      break;
    case GLFW_KEY_Q:
      rescaleToDataRange();
      break;
    case GLFW_KEY_T:
      m_cameraIs3D = !m_cameraIs3D;
      m_camera.setType(m_camera.getType() == Camera::Type::Orthographic ? Camera::Type::Perspective : Camera::Type::Orthographic);
      break;
    case GLFW_KEY_X:
      m_camera.reset();
      break;
    case GLFW_KEY_L:
      m_showLines = !m_showLines;
      m_stateFlags ^= BGFX_STATE_PT_LINES;
      break;
    case GLFW_KEY_I:
      m_showStats = !m_showStats;
      m_debugFlags ^= BGFX_DEBUG_STATS;
      bgfx::setDebug(m_debugFlags);
      break;
    }

    if (m_showScenarioSelection) {
      for (int i = 0; i < (int)ScenarioType::Count && i <= 9; i++) {
        if (key == GLFW_KEY_0 + i) {
          m_selectedScenarioType = (ScenarioType)i;
        }
      }
    }
  }

  void SweApp::onMouseScrolled(float, float dy) { m_camera.onMouseScrolled(dy); }

  void SweApp::onFileDropped(std::string_view path) {
#ifndef __EMSCRIPTEN__
    if (m_showScenarioSelection && m_selectedScenarioType == ScenarioType::Tsunami) {
      std::filesystem::path filepath(path);
      if (filepath.extension() == ".nc") {
        std::string filename(filepath.filename());
        if (filename.find("bath") != std::string::npos) {
          strncpy(m_bathymetryFile, path.data(), sizeof(m_bathymetryFile));
        } else if (filename.find("displ") != std::string::npos) {
          strncpy(m_displacementFile, path.data(), sizeof(m_displacementFile));
        }
      }
    }
#endif
  }

  const bgfx::EmbeddedShader SweApp::shaders[] = {
    BGFX_EMBEDDED_SHADER(vs_swe),
    BGFX_EMBEDDED_SHADER(fs_swe),

    BGFX_EMBEDDED_SHADER_END()
  };

  bgfx::VertexLayout CellVertex::layout;

  void CellVertex::init() { layout.begin().add(bgfx::Attrib::TexCoord0, 1, bgfx::AttribType::Uint8).end(); };

} // namespace App

Core::Application* Core::createApplication(int, char**) { return new App::SweApp(); }
