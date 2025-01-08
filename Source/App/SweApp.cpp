#include "SweApp.hpp"

#include <algorithm>
#include <bx/math.h>
#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <limits>

#include "Blocks/DimensionalSplitting.hpp"
#include "Scenarios/ArtificialTsunamiScenario.hpp"
#include "Scenarios/TestScenario.hpp"
#include "Scenarios/TsunamiScenario.hpp"
#include "swe/fs_swe.bin.h"
#include "swe/vs_swe.bin.h"
#include "Utils.hpp"

namespace App {

  SweApp::SweApp():
    Core::Application("Swe", 1280, 720) {

    m_program = bgfx::createProgram(bgfx::createShader(bgfx::makeRef(vs_swe, sizeof(vs_swe))), bgfx::createShader(bgfx::makeRef(fs_swe, sizeof(fs_swe))), true);
    if (!bgfx::isValid(m_program)) {
      std::cerr << "Failed to create program" << std::endl;
      return;
    }

    CellVertex::init();

    u_gridData    = bgfx::createUniform("u_gridData", bgfx::UniformType::Vec4);
    u_boundaryPos = bgfx::createUniform("u_boundaryPos", bgfx::UniformType::Vec4);
    u_util        = bgfx::createUniform("u_util", bgfx::UniformType::Vec4);
    u_color1      = bgfx::createUniform("u_color1", bgfx::UniformType::Vec4);
    u_color2      = bgfx::createUniform("u_color2", bgfx::UniformType::Vec4);
    u_color3      = bgfx::createUniform("u_color3", bgfx::UniformType::Vec4);
    s_heightMap   = bgfx::createUniform("u_heightMap", bgfx::UniformType::Sampler);

    bgfx::setDebug(m_debugFlags);
    bgfx::reset(m_windowSize.x, m_windowSize.y, m_resetFlags);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, colorToInt(m_clearColor));
  }

  SweApp::~SweApp() {
    if (isBlockLoaded()) {
      destroyBlock();
    }
    destroyProgram();
  }

  void SweApp::update(float dt) {
    simulate(dt);
    updateGrid();
    updateControls(dt);
    updateCamera();
    render();
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
    if (ImGui::Button("Reset##ResetSimulation")) {
      resetSimulation();
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
          switchView(type);
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

    ImGui::DragFloat("Time Scale", &m_timeScale, 0.1f, 0.0f, 1.0f / dt, "%.1f");

    ImGui::DragFloat("End Simulation Time", &m_endSimulationTime, 0.5f, 0.0f, std::numeric_limits<float>::max(), "%.1f");

    ImGui::SeparatorText("Visualization");

    ImGui::BeginDisabled(m_autoScaleDataRange);
    if (ImGui::DragFloat2("Data Range", m_util, 0.01f, 0.0f, 0.0f, "%.2f")) {
      m_util.y = std::max(m_util.x, m_util.y);
    }

    ImGui::SameLine();
    if (ImGui::Button("Rescale")) {
      setUtilDataRange();
    }
    ImGui::EndDisabled();

    if (ImGui::DragFloat("Value Scale", &m_util.z, 1.0f, 0.0f, 0.0f, "%.0f", ImGuiSliderFlags_NoRoundToFormat)) {
      setCameraTargetCenter();
    }

#ifndef NDEBUG
    ImGui::BeginDisabled();
    if (ImGui::DragFloat2("Near/Far Clip", m_cameraClipping, 0.1f)) {
      m_cameraClipping.x = std::max(0.1f, std::min(m_cameraClipping.x, m_cameraClipping.y - 0.1f));
      m_cameraClipping.y = std::max(m_cameraClipping.x + 0.1f, m_cameraClipping.y);
    }
    ImGui::EndDisabled();
#endif

    if (ImGui::TreeNode("Color Controls")) {
      ImGui::ColorEdit3("Color 1 (low)", m_color1, ImGuiColorEditFlags_NoAlpha);
      ImGui::ColorEdit3("Color 2 (mid)", m_color2, ImGuiColorEditFlags_NoAlpha);
      ImGui::ColorEdit3("Color 3 (high)", m_color3, ImGuiColorEditFlags_NoAlpha);
      if (ImGui::ColorEdit3("Background", m_clearColor)) {
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, colorToInt(m_clearColor));
      }
      ImGui::TreePop();
    }

    ImGui::SeparatorText("Camera");

    if (ImGui::Checkbox("3D", &m_cameraIs3D)) {
      m_camera.setType(m_camera.getType() == Camera::Type::Orthographic ? Camera::Type::Perspective : Camera::Type::Orthographic);
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset##ResetCamera")) {
      m_camera.reset();
    }

    ImGui::SameLine();
    if (ImGui::Button("Recenter")) {
      m_camera.recenter();
    }

    ImGui::SeparatorText("Options");

#ifndef NDEBUG
    if (ImGui::Checkbox("Stats", &m_showStats)) {
      m_debugFlags ^= BGFX_DEBUG_STATS;
      bgfx::setDebug(m_debugFlags);
    }
#endif

    ImGui::SameLine();
    if (ImGui::Checkbox("Wireframe", &m_showLines)) {
      m_stateFlags ^= BGFX_STATE_PT_LINES;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Autoscale", &m_autoScaleDataRange);

#ifndef __EMSCRIPTEN__
    ImGui::SeparatorText("Performance");

    static bool vsync = m_resetFlags & BGFX_RESET_VSYNC;
    if (ImGui::Checkbox("VSync", &vsync)) {
      m_resetFlags ^= BGFX_RESET_VSYNC;
      bgfx::reset(m_windowSize.x, m_windowSize.y, m_resetFlags);
    }
#endif

    ImGui::SameLine();
    ImGui::TextDisabled("FPS: %.0f", ImGui::GetIO().Framerate);

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
  D         : auto scale data range
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

  bool SweApp::isBlockLoaded() { return m_scenario && m_block; }

  void SweApp::destroyBlock() {
    delete m_scenario;
    delete m_block;
    delete m_vertices;
    delete m_indices;
    delete m_heightMapData;

    m_block         = nullptr;
    m_scenario      = nullptr;
    m_vertices      = nullptr;
    m_indices       = nullptr;
    m_heightMapData = nullptr;

    bgfx::destroy(m_vbh);
    bgfx::destroy(m_ibh);
    bgfx::destroy(m_heightMap);
  }

  void SweApp::destroyProgram() {
    bgfx::destroy(u_gridData);
    bgfx::destroy(u_boundaryPos);
    bgfx::destroy(u_util);
    bgfx::destroy(u_color1);
    bgfx::destroy(u_color2);
    bgfx::destroy(u_color3);
    bgfx::destroy(s_heightMap);

    bgfx::destroy(m_program);
  }

  void SweApp::initializeBlock() {
    if (isBlockLoaded()) {
      destroyBlock();
    }

    switch (m_scenarioType) {
    case ScenarioType::Test:
      m_scenario = new Scenarios::TestScenario(m_boundaryType, m_dimensions.x);
      m_util.z   = 1.0f; // Value scale
      break;
    case ScenarioType::ArtificialTsunami:
      m_scenario = new Scenarios::ArtificialTsunamiScenario(m_boundaryType);
      m_util.z   = 1000.0f;
      break;
#ifndef __EMSCRIPTEN__
    case ScenarioType::Tsunami: {
      const auto* s = new Scenarios::TsunamiScenario(m_bathymetryFile, m_displacementFile, m_boundaryType, m_dimensions.x, m_dimensions.y);
      if (s->success()) {
        m_scenario = s;
        m_util.z   = 1.0f;
        break;
      }
      std::cerr << "Failed to load Tsunami scenario" << std::endl;
      delete s;
      m_scenarioType = ScenarioType::None;
      [[fallthrough]];
    }
#endif
    case ScenarioType::None:
      m_dimensions     = {};
      m_gridData       = {};
      m_boundaryPos    = {};
      m_util           = {};
      m_cameraClipping = {};
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

    createGrid({nx, ny});

    updateGrid();
    setUtilDataRange();
    setCameraTargetCenter();
    m_camera.reset();
    m_autoScaleDataRange = true;

    m_endSimulationTime = 0.0;
  }

  void SweApp::createGrid(Vec2i n) {
    m_vertices = new CellVertex[n.x * n.y];

#if 0 // TriStip
    m_indices = new uint32_t[2 * (n.x + 1) * (n.y - 1) - 2];
    int index = 0;

    for (int j = 0; j < n.y - 1; j++) {
      for (int i = 0; i < n.x; i++) {
        m_indices[index++] = j * n.x + i;
        m_indices[index++] = (j + 1) * n.x + i;
      }
      if (j < n.y - 2) {
        m_indices[index++] = (j + 1) * n.x + (n.x - 1);
        m_indices[index++] = (j + 1) * n.x;
      }
    }
    m_stateFlags |= BGFX_STATE_PT_TRISTRIP;
    assert(index == 2 * (n.x + 1) * (n.y - 1) - 2); // DEBUG
#else // TriList
    m_indices = new uint32_t[6 * (n.x - 1) * (n.y - 1)];
    int index = 0;

    for (int j = 0; j < n.y - 1; j++) {
      for (int i = 0; i < n.x - 1; i++) {
        uint32_t topLeft     = j * n.x + i;
        uint32_t topRight    = topLeft + 1;
        uint32_t bottomLeft  = (j + 1) * n.x + i;
        uint32_t bottomRight = bottomLeft + 1;
        m_indices[index++]   = topLeft;
        m_indices[index++]   = bottomLeft;
        m_indices[index++]   = topRight;
        m_indices[index++]   = topRight;
        m_indices[index++]   = bottomLeft;
        m_indices[index++]   = bottomRight;
      }
    }
    m_stateFlags &= ~BGFX_STATE_PT_TRISTRIP;
    assert(index == 6 * (n.x - 1) * (n.y - 1)); // DEBUG
#endif

    m_vbh = bgfx::createVertexBuffer(bgfx::makeRef(m_vertices, n.x * n.y * sizeof(CellVertex)), CellVertex::layout);
    m_ibh = bgfx::createIndexBuffer(bgfx::makeRef(m_indices, index * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);

    m_heightMap     = bgfx::createTexture2D(n.x, n.y, false, 1, bgfx::TextureFormat::R32F, BGFX_TEXTURE_NONE);
    m_heightMapData = new float[n.x * n.y];
  }

  void SweApp::loadScenario() {
    m_scenarioType          = m_selectedScenarioType;
    m_dimensions            = m_selectedDimensions;
    m_simulationTime        = 0.0;
    m_playing               = false;
    m_showScenarioSelection = false;

    initializeBlock();
  }

  void SweApp::resetSimulation() {
    if (!isBlockLoaded())
      return;

    m_block->initialiseScenario(m_block->getOffsetX(), m_block->getOffsetY(), *m_scenario);
    m_simulationTime = 0.0;
    m_playing        = false;

    setBlockBoundaryType(m_block, m_boundaryType);
  }

  void SweApp::setUtilDataRange() {
    if (std::abs(m_minMax.x - m_minMax.y) < 0.02f) {
      float mid = (m_minMax.x + m_minMax.y) * 0.5f;
      m_util.x  = mid - 0.01f;
      m_util.y  = mid + 0.01f;
    } else {
      m_util.x = m_minMax.x;
      m_util.y = m_minMax.y;
    }
  }

  void SweApp::setCameraTargetCenter() {
    if (!isBlockLoaded())
      return;
    Vec3f center;
    center.x = (m_boundaryPos.x + m_boundaryPos.y) * 0.5f;
    center.y = (m_boundaryPos.z + m_boundaryPos.w) * 0.5f;
    center.z = (float)getScenarioValue(m_scenario, m_viewType, RealType(center.x), RealType(center.y)) * m_util.z;
    m_camera.setTargetCenter(center);
  }

  void SweApp::switchView(ViewType viewType) {
    m_viewType = viewType;
    setCameraTargetCenter();
    updateGrid();
    setUtilDataRange();
  }

  void SweApp::switchBoundary(BoundaryType boundaryType) {
    m_boundaryType = boundaryType;
    setBlockBoundaryType(m_block, m_boundaryType);
  }

  void SweApp::simulate(float dt) {
    if (!isBlockLoaded() || !m_playing) {
      return;
    }

    m_block->setGhostLayer();

    RealType scaleFactor = RealType(std::min(dt * m_timeScale, 1.0f));

    m_block->computeMaxTimeStep();
    RealType maxTimeStep = m_block->getMaxTimeStep();
    maxTimeStep *= scaleFactor;
    m_block->simulateTimeStep(maxTimeStep);

    m_simulationTime += (float)maxTimeStep;

    if (m_endSimulationTime > 0.0 && m_simulationTime >= m_endSimulationTime) {
      m_playing = false;
    }
  }

  void SweApp::updateGrid() {
    if (!isBlockLoaded())
      return;

    int nx = m_dimensions.x;
    int ny = m_dimensions.y;

    Vec2f minMax = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};

    for (int j = 0; j < ny; j++) {
      for (int i = 0; i < nx; i++) {
        float value = getBlockValue(m_block, m_viewType, i, j);
        minMax.x    = std::min(minMax.x, value);
        minMax.y    = std::max(minMax.y, value);

        m_heightMapData[j * nx + i] = value;
      }
    }

    m_minMax = minMax;

    bgfx::updateTexture2D(m_heightMap, 0, 0, 0, 0, nx, ny, bgfx::makeRef(m_heightMapData, sizeof(float) * nx * ny));
  }

  void SweApp::updateControls(float) {
    if (m_autoScaleDataRange) {
      setUtilDataRange();
    }
  }

  void SweApp::updateCamera() {
    if (!isBlockLoaded() || m_windowSize.x <= 0 || m_windowSize.y <= 0)
      return;

    m_camera.setMouseOverUI(ImGui::GetIO().WantCaptureMouse);
    m_camera.update();

    // Calculate camera clipping planes so that the grid is always visible
    float maxDim       = std::max(m_boundaryPos.y - m_boundaryPos.x, m_boundaryPos.w - m_boundaryPos.z);
    float centerZ      = m_camera.getTargetCenter().z / m_util.z;
    Vec3f offset       = m_camera.getTargetOffset();
    float maxOffset    = std::max(std::max(std::abs(offset.x), std::abs(offset.y)), std::abs(offset.z));
    float maxDist      = std::max(std::abs(m_minMax.x - centerZ), std::abs(m_minMax.y - centerZ)) * std::abs(m_util.z);
    m_cameraClipping.x = maxDim * 0.005f;
    m_cameraClipping.y = m_camera.getZoom() * maxDim + std::max(maxDim, maxDist) + maxOffset * 2.0f;

    m_camera.applyViewProjection();
  }

  void SweApp::render() {
    bgfx::touch(0);

    if (isBlockLoaded()) {
      bgfx::setIndexBuffer(m_ibh);
      bgfx::setVertexBuffer(0, m_vbh);
      bgfx::setTexture(0, s_heightMap, m_heightMap);
    }

    bgfx::setUniform(u_gridData, m_gridData);
    bgfx::setUniform(u_boundaryPos, m_boundaryPos);
    bgfx::setUniform(u_util, m_util);
    bgfx::setUniform(u_color1, m_color1);
    bgfx::setUniform(u_color2, m_color2);
    bgfx::setUniform(u_color3, m_color3);

    bgfx::setState(m_stateFlags);
    bgfx::submit(0, m_program);

    bgfx::frame();
  }

  void SweApp::onResize(int, int) {}

  void SweApp::onKeyPressed(Core::KeyCode key) {
    switch (key) {
    case Core::Key::C:
      m_showControls = !m_showControls;
      break;
    case Core::Key::S:
      m_showScenarioSelection = !m_showScenarioSelection;
      break;
    case Core::Key::Enter:
      if (m_showScenarioSelection)
        loadScenario();
      break;
    case Core::Key::Space:
      m_playing = !m_playing;
      break;
    case Core::Key::R:
      resetSimulation();
      break;
    case Core::Key::H:
      switchView(ViewType::H);
      break;
    case Core::Key::U:
      switchView(ViewType::Hu);
      break;
    case Core::Key::V:
      switchView(ViewType::Hv);
      break;
    case Core::Key::B:
      switchView(ViewType::B);
      break;
    case Core::Key::A:
      switchView(ViewType::HPlusB);
      break;
    case Core::Key::O:
      switchBoundary(BoundaryType::Outflow);
      break;
    case Core::Key::W:
      switchBoundary(BoundaryType::Wall);
      break;
    case Core::Key::Q:
      setUtilDataRange();
      break;
    case Core::Key::T:
      m_cameraIs3D = !m_cameraIs3D;
      m_camera.setType(m_camera.getType() == Camera::Type::Orthographic ? Camera::Type::Perspective : Camera::Type::Orthographic);
      break;
    case Core::Key::X:
      m_camera.reset();
      break;
    case Core::Key::D:
      m_autoScaleDataRange = !m_autoScaleDataRange;
      break;
    case Core::Key::L:
      m_showLines = !m_showLines;
      m_stateFlags ^= BGFX_STATE_PT_LINES;
      break;
    case Core::Key::I:
      m_showStats = !m_showStats;
      m_debugFlags ^= BGFX_DEBUG_STATS;
      bgfx::setDebug(m_debugFlags);
      break;
    }

    if (m_showScenarioSelection) {
      for (int i = 0; i < (int)ScenarioType::Count && i <= 9; i++) {
        if (key == Core::Key::D0 + i) {
          m_selectedScenarioType = (ScenarioType)i;
        }
      }
    }
  }

  void SweApp::onMouseScrolled(float, float dy) { m_camera.onMouseScrolled(dy); }

  void SweApp::onFileDropped([[maybe_unused]] std::string_view path) {
#ifndef __EMSCRIPTEN__
    if (m_showScenarioSelection && m_selectedScenarioType == ScenarioType::Tsunami) {
      std::filesystem::path filepath(path);
      if (filepath.extension() == ".nc") {
        std::string usablePath = removeDriveLetter(path.data()); // Remove "C:" on windows
        std::string filename   = filepath.filename().string();
        if (filename.find("bath") != std::string::npos) {
          strncpy(m_bathymetryFile, usablePath.c_str(), sizeof(m_bathymetryFile));
        } else if (filename.find("displ") != std::string::npos) {
          strncpy(m_displacementFile, usablePath.c_str(), sizeof(m_displacementFile));
        }
      }
    }
#endif
  }

  bgfx::VertexLayout CellVertex::layout;

  void CellVertex::init() { layout.begin().add(bgfx::Attrib::Position, 1, bgfx::AttribType::Uint8).end(); };

} // namespace App

Core::Application* Core::createApplication(int, char**) { return new App::SweApp(); }
