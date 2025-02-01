#include "SweApp.hpp"

#include <algorithm>
#include <bx/math.h>
#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <limits>

#include "Blocks/DimensionalSplitting.hpp"
#include "Scenarios/ArtificialTsunamiScenario.hpp"
#include "Scenarios/NetCDFScenario.hpp"
#include "Scenarios/RealisticScenario.hpp"
#include "Scenarios/TestScenario.hpp"
#include "Shaders/swe/fs_swe.bin.h"
#include "Shaders/swe/vs_swe.bin.h"
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
    u_dataRanges  = bgfx::createUniform("u_dataRanges", bgfx::UniformType::Vec4);
    u_util        = bgfx::createUniform("u_util", bgfx::UniformType::Vec4);
    u_color1      = bgfx::createUniform("u_color1", bgfx::UniformType::Vec4);
    u_color2      = bgfx::createUniform("u_color2", bgfx::UniformType::Vec4);
    u_color3      = bgfx::createUniform("u_color3", bgfx::UniformType::Vec4);
    s_heightMap   = bgfx::createUniform("u_heightMap", bgfx::UniformType::Sampler);

    bgfx::setViewClear(m_mainView, m_clearFlags, colorToInt(m_clearColor));

    setSelectedScenarioType(ScenarioType::Chile);
    selectScenario();
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

    drawControlWindow(dt);
    drawHelpWindow();

    if (m_showScenarioSelection) {
      drawScenarioSelectionWindow();
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
    bgfx::destroy(u_dataRanges);
    bgfx::destroy(u_util);
    bgfx::destroy(u_color1);
    bgfx::destroy(u_color2);
    bgfx::destroy(u_color3);
    bgfx::destroy(s_heightMap);

    bgfx::destroy(m_program);
  }

  bool SweApp::loadScenario() {
    switch (m_scenarioType) {
#ifdef ENABLE_NETCDF
    case ScenarioType::NetCDF: {
      m_scenario = new Scenarios::NetCDFScenario(m_bathymetryFile, m_displacementFile, m_boundaryType);
      break;
    }
#endif
    case ScenarioType::Tohoku: {
      m_scenario = new Scenarios::RealisticScenario(Scenarios::RealisticScenarioType::Tohoku, m_boundaryType);
      break;
    }
    case ScenarioType::TohokuZoomed: {
      m_scenario = new Scenarios::RealisticScenario(Scenarios::RealisticScenarioType::TohokuZoomed, m_boundaryType);
      break;
    }
    case ScenarioType::Chile: {
      m_scenario = new Scenarios::RealisticScenario(Scenarios::RealisticScenarioType::Chile, m_boundaryType);
      break;
    }
    case ScenarioType::ArtificialTsunami: {
      m_scenario = new Scenarios::ArtificialTsunamiScenario(m_boundaryType);
      break;
    }
#ifndef NDEBUG
    case ScenarioType::Test: {
      m_scenario = new Scenarios::TestScenario(m_boundaryType, m_dimensions.x);
      break;
    }
#endif
    case ScenarioType::None: {
      setNoneScenario();
      m_message = "";
      break;
    }
    default:
      assert(false);
      break;
    }

    if (m_scenario && !m_scenario->loadSuccess()) {
      delete m_scenario;
      m_scenario = nullptr;
      setNoneScenario();
      warn("Failed loading scenario");
    }

    return m_scenarioType != ScenarioType::None;
  }

  void SweApp::setNoneScenario() {
    m_scenarioType = ScenarioType::None;
    m_dimensions   = {};
    m_gridData     = {};
    m_boundaryPos  = {};
    m_dataRanges   = {};
    m_util         = {};
  }

  bool SweApp::initializeBlock(bool silent) {
    if (isBlockLoaded()) {
      destroyBlock();
    }

    bool loaded = loadScenario();
    if (!loaded) {
      return false;
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

#ifndef NDEBUG
    std::cout << "Loading block with scenario: " << scenarioTypeToString(m_scenarioType) << std::endl;
    std::cout << "  nx: " << nx << ", ny: " << ny << std::endl;
    std::cout << "  dx: " << dx << ", dy: " << dy << std::endl;
    std::cout << "  Left: " << left << ", Right: " << right << ", Bottom: " << bottom << ", Top: " << top << std::endl;
#endif

    m_block = new Blocks::DimensionalSplittingBlock(nx, ny, dx, dy);
    m_block->initialiseScenario(left, bottom, *m_scenario);
    m_block->setGhostLayer();

    createGrid({nx, ny});

    if (!silent) {
      setColorAndValueScale();
      resetCamera();
      resetDisplacementData();
    }

    m_message = "";

    return true;
  }

  void SweApp::createGrid(Vec2i n) {
    m_vertices = new CellVertex[n.x * n.y];

    for (int j = 0; j < n.y; j++) {
      for (int i = 0; i < n.x; i++) {
        m_vertices[j * n.x + i].isDry = m_block->getBathymetry()[j + 1][i + 1] > RealType(0) ? 255 : 0;
      }
    }

#if 0 // TriStrip
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
    assert(index == 2 * (n.x + 1) * (n.y - 1) - 2);
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
    assert(index == 6 * (n.x - 1) * (n.y - 1));
#endif

    m_vbh = bgfx::createVertexBuffer(bgfx::makeRef(m_vertices, n.x * n.y * sizeof(CellVertex)), CellVertex::layout);
    m_ibh = bgfx::createIndexBuffer(bgfx::makeRef(m_indices, index * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);

    m_heightMap     = bgfx::createTexture2D(n.x, n.y, false, 1, bgfx::TextureFormat::R32F, BGFX_TEXTURE_NONE);
    m_heightMapData = new float[n.x * n.y];
  }

  bool SweApp::selectScenario(bool silentHint) {
    bool silent = silentHint && m_scenarioType == m_selectedScenarioType && m_dimensions == m_selectedDimensions;

    m_scenarioType          = m_selectedScenarioType;
    m_dimensions            = m_selectedDimensions;
    m_simulationTime        = 0.0;
    m_playing               = false;
    m_showScenarioSelection = false;

    return initializeBlock(silent);
  }

  void SweApp::startStopSimulation() {
    m_playing = !m_playing;
    m_message = "";
  }

  void SweApp::resetSimulation() {
    if (!isBlockLoaded())
      return;

    m_block->initialiseScenario(m_block->getOffsetX(), m_block->getOffsetY(), *m_scenario);
    m_simulationTime = 0.0;
    m_playing        = false;

    setBlockBoundaryType(m_block, m_boundaryType);
  }

  void SweApp::setWetDataRange() {
    if (std::abs(m_minMaxWet.x - m_minMaxWet.y) < 0.02f) {
      float mid      = (m_minMaxWet.x + m_minMaxWet.y) * 0.5f;
      m_dataRanges.x = mid - 0.01f;
      m_dataRanges.y = mid + 0.01f;
    } else {
      m_dataRanges.x = m_minMaxWet.x;
      m_dataRanges.y = m_minMaxWet.y;
    }
  }

  void SweApp::resetCamera() {
    setCameraTargetCenter();
    m_camera.reset();
  }

  void SweApp::setCameraTargetCenter() {
    if (!isBlockLoaded())
      return;
    Vec3f center;
    center.x = (m_boundaryPos.x + m_boundaryPos.y) * 0.5f;
    center.y = (m_boundaryPos.z + m_boundaryPos.w) * 0.5f;
    if (m_viewType != ViewType::HPlusB) {
      center.z = (float)getScenarioValue(m_scenario, m_viewType, RealType(center.x), RealType(center.y)) * m_util.x;
    } else {
      center.z = 0.0f;
    }
    m_camera.setTargetCenter(center);
  }

  void SweApp::setSelectedScenarioType(ScenarioType scenarioType) {
    m_selectedScenarioType = scenarioType;

    switch (scenarioType) {
    case ScenarioType::Tohoku:
      m_selectedDimensions = {350, 200};
      break;
    case ScenarioType::TohokuZoomed:
      m_selectedDimensions = {265, 200};
      break;
    case ScenarioType::Chile:
      m_selectedDimensions = {400, 300};
      break;
    case ScenarioType::ArtificialTsunami:
      m_selectedDimensions = {100, 100};
      break;
#ifndef NDEBUG
    case ScenarioType::Test:
      m_selectedDimensions = {20, 20};
      break;
#endif
    default:
      m_selectedDimensions = {250, 250};
      break;
    }
  }

  void SweApp::setColorAndValueScale(bool resetValueScale) {
    if (!isBlockLoaded())
      return;

    if (resetValueScale) {
      m_util.y = getInitialZValueScale(m_scenarioType).y;
      m_util.z = getInitialZValueScale(m_scenarioType).x;
      m_util.w = 10000.0f;
    }
    m_util.x = m_viewType == ViewType::H || m_viewType == ViewType::B ? m_util.z : m_util.w;

    if (m_viewType != ViewType::HPlusB) {
      updateGrid();
      setWetDataRange();
    } else {
      m_dataRanges.x = -0.01f;
      m_dataRanges.y = 0.01f;
    }
  }

  void SweApp::switchView(ViewType viewType) {
    m_viewType = viewType;
    setColorAndValueScale(false);
    setCameraTargetCenter();
    m_message = "";
  }

  void SweApp::switchBoundary(BoundaryType boundaryType) {
    m_boundaryType = boundaryType;
    setBlockBoundaryType(m_block, m_boundaryType);
  }

  void SweApp::toggleWireframe() { m_stateFlags ^= BGFX_STATE_PT_LINES; }

  void SweApp::toggleStats() {
    m_debugFlags ^= BGFX_DEBUG_STATS;
    bgfx::setDebug(m_debugFlags);
  }

  void SweApp::toggleVsync() {
    m_resetFlags ^= BGFX_RESET_VSYNC;
    bgfx::reset(m_windowSize.x, m_windowSize.y, m_resetFlags);
  }

  void SweApp::resetDisplacementData() {
    m_displacementPosition = {0.0f, 0.0f};
    m_displacementRadius   = 100000.0f;
    m_displacementHeight   = 10.0f;
  }

  void SweApp::applyDisplacement() {
    if (!isBlockLoaded())
      return;

    if (!m_customDisplacement) {
      m_block->setWaterHeight([](RealType x, RealType y) -> RealType {
        SweApp* app = static_cast<SweApp*>(Core::Application::get());
        return getBlockValue(app->m_block, ViewType::H, x, y) + app->m_scenario->getDisplacement(x, y);
      });
    } else {
      Vec4f& b = m_boundaryPos;
      Vec2f& d = m_displacementPosition;
      Vec2f  r = {m_displacementRadius / (b.y - b.x), m_displacementRadius / (b.w - b.z)};
      if (d.x - r.x < -0.5f || d.x + r.x > 0.5f || d.y - r.y < -0.5f || d.y + r.y > 0.5f) {
        r *= 1.2f;
        d.x = bx::clamp(d.x, -0.5f + r.x, 0.5f - r.x);
        d.y = bx::clamp(d.y, -0.5f + r.y, 0.5f - r.y);
      }
      m_block->setWaterHeight([](RealType x, RealType y) -> RealType {
        SweApp*  app    = static_cast<SweApp*>(Core::Application::get());
        RealType height = getBlockValue(app->m_block, ViewType::H, x, y);
        if (app->m_scenario->getWaterHeight(x, y) > 0) {
          Vec4f& bound  = app->m_boundaryPos;
          float  displA = app->m_displacementHeight;
          float  displP = app->m_displacementRadius * 2.0f;
          Vec2f  displC = {(bound.x + bound.y) * 0.5f, (bound.z + bound.w) * 0.5f};
          displC += app->m_displacementPosition * Vec2f(bound.y - bound.x, bound.w - bound.z);
          height += Scenarios::ArtificialTsunamiScenario::getCustomDisplacement(x, y, {displA, displP, displC.x, displC.y});
        }
        return height;
      });
    }
  }

  void SweApp::warn(const char* message) {
    m_message = message;
    std::cerr << message << std::endl;
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

    if (m_block->hasError()) {
      warn("Simulation crashed");
      resetSimulation();
      return;
    }

    m_simulationTime += (float)maxTimeStep;
  }

  void SweApp::updateGrid() {
    if (!isBlockLoaded())
      return;

    int nx = m_dimensions.x;
    int ny = m_dimensions.y;

    Vec2f minMaxWet = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
    Vec2f minMaxDry = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};

    for (int j = 0; j < ny; j++) {
      for (int i = 0; i < nx; i++) {
        float value = (float)getBlockValue(m_block, m_viewType, i + 1, j + 1);
        int   index = j * nx + i;

        if (!m_vertices[index].isDry) {
          minMaxWet.x = std::min(minMaxWet.x, value);
          minMaxWet.y = std::max(minMaxWet.y, value);
        } else {
          minMaxDry.x = std::min(minMaxDry.x, value);
          minMaxDry.y = std::max(minMaxDry.y, value);
        }

        m_heightMapData[index] = value;
      }
    }

    m_minMaxWet    = minMaxWet;
    m_dataRanges.z = minMaxDry.x;
    m_dataRanges.w = minMaxDry.y;

    bgfx::updateTexture2D(m_heightMap, 0, 0, 0, 0, nx, ny, bgfx::makeRef(m_heightMapData, sizeof(float) * nx * ny));
  }

  void SweApp::updateControls(float) {
    if (m_autoScaleDataRange) {
      setWetDataRange();
    }
    if (m_dataRanges.z == m_dataRanges.w) {
      m_dataRanges.w += 0.01f;
    }
  }

  void SweApp::updateCamera() {
    if (!isBlockLoaded() || m_windowSize.x <= 0 || m_windowSize.y <= 0)
      return;

    m_camera.setMouseOverUI(ImGui::GetIO().WantCaptureMouse);
    m_camera.update();

    // Calculate camera clipping planes so that the grid is always visible
    float maxDim       = std::max(m_boundaryPos.y - m_boundaryPos.x, m_boundaryPos.w - m_boundaryPos.z);
    float centerZ      = m_camera.getTargetCenter().z / m_util.x;
    Vec3f offset       = m_camera.getTargetOffset();
    float maxOffset    = std::max(std::max(std::abs(offset.x), std::abs(offset.y)), std::abs(offset.z));
    float maxScale     = std::max(std::abs(m_util.x), std::abs(m_util.y));
    Vec2f minMaxValue  = {std::min(m_minMaxWet.x, m_dataRanges.z), std::max(m_minMaxWet.y, m_dataRanges.w)};
    float maxDist      = std::max(std::abs(minMaxValue.x - centerZ), std::abs(minMaxValue.y - centerZ)) * maxScale;
    m_cameraClipping.x = maxDim * 0.005f;
    m_cameraClipping.y = m_camera.getZoom() * maxDim + std::max(maxDim, maxDist) + maxOffset * 2.0f;

    m_camera.applyViewProjection(m_mainView);
  }

  void SweApp::render() {
    bgfx::touch(m_mainView);

    if (isBlockLoaded()) {
      bgfx::setIndexBuffer(m_ibh);
      bgfx::setVertexBuffer(0, m_vbh);
      bgfx::setTexture(0, s_heightMap, m_heightMap);

      bgfx::setUniform(u_gridData, m_gridData);
      bgfx::setUniform(u_boundaryPos, m_boundaryPos);
      bgfx::setUniform(u_dataRanges, m_dataRanges);
      bgfx::setUniform(u_util, m_util);
      bgfx::setUniform(u_color1, m_color1);
      bgfx::setUniform(u_color2, m_color2);
      bgfx::setUniform(u_color3, m_color3);
    }

    bgfx::setState(m_stateFlags);
    bgfx::submit(m_mainView, m_program);

    bgfx::frame();
  }

  void SweApp::drawControlWindow(float dt) {
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::SeparatorText("Simulation");

    if (ImGui::Button("Select Scenario")) {
      m_showScenarioSelection = true;
    }

    ImGui::SameLine();
    ImGui::Text("%s (%dx%d)", scenarioTypeToString(m_scenarioType).c_str(), m_dimensions.x, m_dimensions.y);

    if (!isBlockLoaded()) {
      ImGui::End();
      return;
    }

    if (ImGui::Button("Reset##ResetSimulation")) {
      resetSimulation();
    }

    ImGui::SameLine();
    if (ImGui::Button(!m_playing ? "Start" : "Stop")) {
      startStopSimulation();
    }

    ImGui::SameLine();
    ImGui::Text("Time: %.1f s", m_simulationTime);

    ImGui::SameLine();
    ImGui::TextColored({1, 0, 0, 1}, "%s", m_message);

    if (ImGui::Button("Apply Displacement")) {
      applyDisplacement();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Custom", &m_customDisplacement);

    if (m_customDisplacement) {
      ImGui::SameLine();
      if (ImGui::Button("Reset##ResetDisplacement")) {
        resetDisplacementData();
      }
      ImGui::Indent();
      if (isBlockLoaded()) {
        float domAspect = (m_boundaryPos.y - m_boundaryPos.x) / (m_boundaryPos.w - m_boundaryPos.z);
        if (drawCoordinatePicker2D("Position", m_displacementPosition, {50.0f * domAspect, 50.0f})) {
          applyDisplacement();
        }
        ImGui::SameLine();
        ImGui::Text("Select pos by placing the dot\n\n(Quick apply with right click)");
      }
      float itemWidth = ImGui::CalcItemWidth() / 2.0f - 12.0f;
      ImGui::PushItemWidth(itemWidth);
      ImGui::DragFloat("##DisplHeight", &m_displacementHeight, 0.1f, 0.0f, 0.0f, "%.1f");
      ImGui::SameLine(0.0f, 4.0f);
      ImGui::DragFloat("##DisplRadius", &m_displacementRadius, 250.0f, m_gridData.z, FLT_MAX, "%.0f");
      ImGui::PopItemWidth();
      ImGui::SameLine(0.0f, 4.0f);
      ImGui::Text("Height, Radius");
      ImGui::Unindent();
    }

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

    ImGui::SeparatorText("Visualization");

    ImGui::BeginDisabled(m_autoScaleDataRange);
    if (ImGui::DragFloat2("Data Range", m_dataRanges, 0.01f, 0.0f, 0.0f, "%.2f")) {
      float adjustedX = std::min(m_dataRanges.y - 0.01f, m_dataRanges.x);
      float adjustedY = std::max(m_dataRanges.x + 0.01f, m_dataRanges.y);
      m_dataRanges.x  = adjustedX;
      m_dataRanges.y  = adjustedY;
    }

    ImGui::SameLine();
    if (ImGui::Button("Rescale")) {
      setWetDataRange();
    }
    ImGui::EndDisabled();

    if (m_setFocusValueScale) {
      ImGui::SetKeyboardFocusHere();
      m_setFocusValueScale = false;
    }

    float itemWidth = ImGui::CalcItemWidth() / 2.0f - 2.0f;
    ImGui::PushItemWidth(itemWidth);

    bool   hOrB     = m_viewType == ViewType::H || m_viewType == ViewType::B;
    float* wetScale = hOrB ? &m_util.z : &m_util.w;
    if (ImGui::DragFloat("##ZScaleWet", wetScale, hOrB ? 0.25f : 100.0f, 0.0f, 0.0f, "%.0f", ImGuiSliderFlags_NoRoundToFormat)) {
      m_util.x = *wetScale;
      setCameraTargetCenter();
    }
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::DragFloat("##ZScaleDry", &m_util.y, 0.25f, 0.0f, 0.0f, "%.0f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::PopItemWidth();
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::Text("Z-Scale (Wet, Dry)");

    if (ImGui::Button("Reset##ResetScaling")) {
      setColorAndValueScale();
    }

#ifndef NDEBUG
    ImGui::BeginDisabled();
    ImGui::DragFloat2("Near/Far Clip", m_cameraClipping, 0.1f);
    ImGui::EndDisabled();
#endif

    if (ImGui::TreeNodeEx("Color Controls", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanTextWidth)) {
      ImGui::ColorEdit4("Color 1 (low)", m_color1, ImGuiColorEditFlags_NoAlpha);
      ImGui::ColorEdit4("Color 2 (mid)", m_color2, ImGuiColorEditFlags_NoAlpha);
      ImGui::ColorEdit4("Color 3 (high)", m_color3, ImGuiColorEditFlags_NoAlpha);
      if (ImGui::ColorEdit4("Background", m_clearColor, ImGuiColorEditFlags_NoAlpha)) {
        bgfx::setViewClear(m_mainView, m_clearFlags, colorToInt(m_clearColor));
      }
      if (ImGui::Button("Reset##ResetColors")) {
        m_color1 = {0.0f, 0.0f, 0.0f, 1.0f};
        m_color2 = {0.0f, 0.25f, 1.0f, 1.0f};
        m_color3 = {1.0f, 1.0f, 1.0f, 1.0f};
      }
      ImGui::TreePop();
    }

    ImGui::SeparatorText("Camera");

    if (ImGui::Checkbox("3D", &m_cameraIs3D)) {
      m_camera.setType(m_camera.getType() == Camera::Type::Orthographic ? Camera::Type::Perspective : Camera::Type::Orthographic);
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset##ResetCamera")) {
      resetCamera();
    }

    ImGui::SameLine();
    if (ImGui::Button("Recenter")) {
      m_camera.recenter();
    }

    ImGui::SeparatorText("Options");

#ifndef NDEBUG
    if (ImGui::Checkbox("Stats", &m_showStats)) {
      toggleStats();
    }
    ImGui::SameLine();
#endif

    if (ImGui::Checkbox("Wireframe", &m_showLines)) {
      toggleWireframe();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Autoscale", &m_autoScaleDataRange);

    ImGui::SameLine();
    if (ImGui::Button("Hide")) {
      m_showControls = false;
    }
    ImGui::SetItemTooltip("Unhide windows with 'C'");

#ifndef __EMSCRIPTEN__
    ImGui::SeparatorText("Performance");

    if (ImGui::Checkbox("VSync", &m_vsyncEnabled)) {
      toggleVsync();
    }
#endif

    ImGui::SameLine();
    ImGui::TextDisabled("FPS: %.0f", ImGui::GetIO().Framerate);

    ImGui::End(); // Controls
  }

  void SweApp::drawScenarioSelectionWindow() {
    ImGui::Begin("Scenario Selection", &m_showScenarioSelection, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginCombo("Scenario", scenarioTypeToString(m_selectedScenarioType).c_str())) {
      for (int i = 0; i < (int)ScenarioType::Count; i++) {
        ScenarioType type = (ScenarioType)i;
        if (ImGui::Selectable(scenarioTypeToString(type).c_str(), m_selectedScenarioType == type)) {
          setSelectedScenarioType(type);
        }
      }
      ImGui::EndCombo();
    }

    if (ImGui::InputInt2("Grid Dimensions", m_selectedDimensions)) {
      m_selectedDimensions.x = std::clamp(m_selectedDimensions.x, 2, 2000);
      m_selectedDimensions.y = std::clamp(m_selectedDimensions.y, 2, 2000);
    }

#ifdef ENABLE_NETCDF
    if (m_selectedScenarioType == ScenarioType::NetCDF) {
      ImGui::Text("Drag-drop GEBCO netCDF files generated from ");
      ImGui::SameLine(0.0f, 0.0f);
      ImGui::TextLinkOpenURL("here", "https://download.gebco.net/");

      int fileInputTextFlags = ImGuiInputTextFlags_ElideLeft;
#ifdef __EMSCRIPTEN__
      fileInputTextFlags |= ImGuiInputTextFlags_ReadOnly;
#endif
      ImGui::InputText("Bathymetry File", m_bathymetryFile, sizeof(m_bathymetryFile), fileInputTextFlags);
      ImGui::SetItemTooltip("File name must start with 'gebco' or contain 'bath'");
      ImGui::InputText("Displacement File", m_displacementFile, sizeof(m_displacementFile), fileInputTextFlags);
      ImGui::SetItemTooltip("Optional custom displacement file name must contain 'displ'");
    }
#endif

    if (ImGui::Button("Load Scenario")) {
      selectScenario();
    }

    ImGui::End(); // Scenario Selection
  }

  void SweApp::drawHelpWindow() {
    ImGui::SetNextWindowSize(ImVec2(310, 210), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(310, 140), ImVec2(310, std::min(700, m_windowSize.y)));
    ImGui::SetNextWindowPos(ImVec2(m_windowSize.x - 310, 0));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGui::GetStyle().Colors[ImGuiCol_TitleBg]);

    int helpWindowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing;
    if (ImGui::Begin("Shortcuts", nullptr, helpWindowFlags)) {

      auto addRow = [](const char* key, const char* description) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        int padding = (10 - strlen(key)) / 2;
        ImGui::Text("%*s%s%*s", padding, "", key, padding, "");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", description);
      };

      if (isBlockLoaded()) {
        if (ImGui::BeginTable("ButtonBindingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
          ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed, 65.0f);
          ImGui::TableSetupColumn("Description");
          ImGui::TableHeadersRow();
          if (m_cameraIs3D) {
            addRow("Left", "Rotate camera");
            addRow("Ctrl+Left", "Pan camera");
          } else {
            addRow("Left", "Pan camera");
          }
          addRow("Middle", "Pan camera");
          addRow("Right", "Zoom camera");
          addRow("Scroll", "Zoom camera");
          ImGui::EndTable();
        }
        ImGui::NewLine();
      }

      if (ImGui::BeginTable("KeyBindingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 65.0f);
        ImGui::TableSetupColumn("Description");
        ImGui::TableHeadersRow();
        addRow("C", "Hide windows");
        addRow("S", "Open scenario selection");
        if (m_showScenarioSelection) {
          addRow("0-9", "Select scenario");
          addRow("Enter", "Load selected scenario");
        }
        if (isBlockLoaded()) {
          addRow("Space", "Start/stop simulation");
          addRow("R", "Reset simulation");
          addRow("F", "Apply displacement");
          addRow("G", "Toggle custom displacement");
          addRow("E", "Nav focus on z-value scale");
          addRow("H", "Set view type: Height");
          addRow("U", "Set view type: Momentum U");
          addRow("V", "Set view type: Momentum V");
          addRow("B", "Set view type: Bathymetry");
          addRow("A", "Set view type: H + B");
          addRow("O", "Set boundary type: Outflow");
          addRow("W", "Set boundary type: Wall");
          addRow("Q", "Auto rescale data range");
          addRow("J", "Reset data range and scaling");
          addRow("T", "Switch camera type");
          addRow("X", "Reset camera");
          addRow("M", "Recenter camera");
          addRow("D", "Auto scale data range");
          addRow("L", "Show lines");
        }
        addRow("I", "Show stats");
#ifndef __EMSCRIPTEN__
        addRow("P", "Toggle vsync");
#endif
        addRow("TAB", "Nav focus next item");
        addRow("Shift+TAB", "Nav focus prev item");
        addRow("Ctrl+TAB", "Nav focus next window");
        addRow("Enter", "Nav activate item");
        addRow("ESC", "Nav cancel item");
        ImGui::EndTable();
      }
    }
    ImGui::End();
    ImGui::PopStyleColor();
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
      if (m_showScenarioSelection && !ImGui::GetIO().NavVisible)
        selectScenario();
      break;
    case Core::Key::R:
      resetSimulation();
      break;
    case Core::Key::Space:
      ImGui::SetNavCursorVisible(false);
      startStopSimulation();
      break;
    case Core::Key::F:
      applyDisplacement();
      break;
    case Core::Key::G:
      m_customDisplacement = !m_customDisplacement;
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
      setWetDataRange();
      break;
    case Core::Key::J:
      setColorAndValueScale();
      break;
    case Core::Key::T:
      m_cameraIs3D = !m_cameraIs3D;
      m_camera.setType(m_camera.getType() == Camera::Type::Orthographic ? Camera::Type::Perspective : Camera::Type::Orthographic);
      break;
    case Core::Key::X:
      resetCamera();
      break;
    case Core::Key::M:
      m_camera.recenter();
      break;
    case Core::Key::D:
      m_autoScaleDataRange = !m_autoScaleDataRange;
      break;
    case Core::Key::L:
      m_showLines = !m_showLines;
      toggleWireframe();
      break;
    case Core::Key::I:
      m_showStats = !m_showStats;
      toggleStats();
      break;
#ifndef __EMSCRIPTEN__
    case Core::Key::P:
      m_vsyncEnabled = !m_vsyncEnabled;
      toggleVsync();
      break;
#endif
    case Core::Key::E:
      m_setFocusValueScale = true;
      break;
    }

    if (m_showScenarioSelection) {
      for (int i = 0; i < (int)ScenarioType::Count && i <= 9; i++) {
        if (key == Core::Key::D0 + i) {
          setSelectedScenarioType((ScenarioType)i);
        }
      }
    }
  }

  void SweApp::onMouseScrolled(float, float dy) { m_camera.onMouseScrolled(dy); }

  void SweApp::onFileDropped([[maybe_unused]] const char** paths, [[maybe_unused]] int count) {
#ifdef ENABLE_NETCDF
    if (m_showScenarioSelection) {
      if (m_selectedScenarioType == ScenarioType::NetCDF) {
        for (int i = 0; i < count; i++) {
          addBathDisplFile(paths[i]);
        }
      }
    } else {
      if (count == 2) {
        if (addBathDisplFile(paths[0], -1) && addBathDisplFile(paths[1], 1)) {
          tryAutoLoadNcFiles({250, 250});
        }
      } else if (count == 1) {
        // single bathymetry file
        std::string oldBathFile = m_bathymetryFile;
        if (addBathDisplFile(paths[0], -1)) {
          m_displacementFile[0] = '\0';
          tryAutoLoadNcFiles({250, 250}, oldBathFile == m_bathymetryFile);
        } else if (isBlockLoaded() && m_scenarioType == ScenarioType::NetCDF) {
          // single displacement file
          if (addBathDisplFile(paths[0], 1)) {
            tryAutoLoadNcFiles(m_dimensions);
          }
        }
      }
    }
#endif
  }

  void SweApp::tryAutoLoadNcFiles([[maybe_unused]] Vec2i dimensions, [[maybe_unused]] bool silent) {
#ifdef ENABLE_NETCDF
    auto typeBackup        = m_selectedScenarioType;
    auto dimensionsBackup  = m_selectedDimensions;
    m_selectedScenarioType = ScenarioType::NetCDF;
    m_selectedDimensions   = dimensions;
    if (!selectScenario(silent)) {
      m_bathymetryFile[0]    = '\0';
      m_displacementFile[0]  = '\0';
      m_selectedScenarioType = typeBackup;
      m_selectedDimensions   = dimensionsBackup;
    }
#endif
  }

  bool SweApp::addBathDisplFile([[maybe_unused]] std::string_view path, [[maybe_unused]] int select) {
#ifdef ENABLE_NETCDF
    std::filesystem::path filepath(path);
    if (filepath.extension() == ".nc") {
      std::string usablePath = removeDriveLetter(path.data()); // Remove "C:" on windows
      std::string filename   = filepath.filename().string();
      if (select <= 0 && (filename.find("bath") != std::string::npos || filename.starts_with("gebco"))) {
        strncpy(m_bathymetryFile, usablePath.c_str(), sizeof(m_bathymetryFile) - 1);
        return true;
      } else if (select >= 0 && filename.find("displ") != std::string::npos) {
        strncpy(m_displacementFile, usablePath.c_str(), sizeof(m_displacementFile) - 1);
        return true;
      }
    }
#endif
    return false;
  }

  bgfx::VertexLayout CellVertex::layout;

  void CellVertex::init() { layout.begin().add(bgfx::Attrib::Position, 1, bgfx::AttribType::Uint8, true).end(); };

} // namespace App

Core::Application* Core::createApplication(int, char**) { return new App::SweApp(); }
