#include "Blocks/DimensionalSplitting.hpp"
#include "Camera.hpp"
#include "Core/Application.hpp"
#include "Types/ScenarioType.hpp"
#include "Types/ViewType.hpp"

namespace App {

  struct CellVertex {
    uint8_t isDry = 0;

    static bgfx::VertexLayout layout;
    static void               init();
  };

  class SweApp: public Core::Application {
  public:
    SweApp();

    ~SweApp() override;

    void update(float dt) override;
    void updateImGui(float dt) override;

    void onResize(int width, int height) override;
    void onKeyPressed(Core::KeyCode key) override;
    void onMouseScrolled(float dx, float dy) override;
    void onFileDropped(std::string_view path) override;

  private:
    bool isBlockLoaded();
    void destroyBlock();
    void destroyProgram();
    bool loadScenario();
    void initializeBlock();
    void createGrid(Vec2i n);
    void selectScenario();
    void startSimulation();
    void resetSimulation();
    void setUtilDataRange();
    void resetCamera();
    void setCameraTargetCenter();
    void setSelectedScenarioType(ScenarioType scenarioType);
    void switchView(ViewType viewType);
    void switchBoundary(BoundaryType boundaryType);
    void toggleWireframe();
    void toggleStats();
    void toggleVsync();
    void applyDisplacement();
    void warn(const char* message);

    void simulate(float dt);
    void updateGrid();
    void updateControls(float dt);
    void updateCamera();
    void render();

  private:
    bgfx::ProgramHandle m_program;

    bgfx::VertexBufferHandle m_vbh;
    bgfx::IndexBufferHandle  m_ibh;

    bgfx::UniformHandle u_gridData;
    bgfx::UniformHandle u_boundaryPos;
    bgfx::UniformHandle u_util;

    bgfx::UniformHandle u_color1;
    bgfx::UniformHandle u_color2;
    bgfx::UniformHandle u_color3;

    bgfx::UniformHandle s_heightMap;
    bgfx::TextureHandle m_heightMap;

    CellVertex* m_vertices      = nullptr;
    uint32_t*   m_indices       = nullptr;
    float*      m_heightMapData = nullptr;

    Vec4f m_gridData;    // x: nx, y: ny, z: dx, w: dy
    Vec4f m_boundaryPos; // x: left, y: right, z: bottom, w: top
    Vec2f m_minMax;      // x: minVal, y: maxVal
    Vec4f m_util;        // x: colorMinVal, y: colorMaxVal, z: valueScale

    Vec4f m_color1 = {0.0f, 0.0f, 0.0f, 1.0f};
    Vec4f m_color2 = {0.0f, 0.25f, 1.0f, 1.0f};
    Vec4f m_color3 = {1.0f, 1.0f, 1.0f, 1.0f};

    Vec2f m_cameraClipping = {0.1f, 1000.0f};
    Vec4f m_clearColor     = {0.16f, 0.17f, 0.19f, 1.0f};

    Blocks::Block*             m_block    = nullptr;
    const Scenarios::Scenario* m_scenario = nullptr;

    ScenarioType m_scenarioType = ScenarioType::None;
    Vec2i        m_dimensions;

    ViewType     m_viewType          = ViewType::HPlusB;
    BoundaryType m_boundaryType      = BoundaryType::Outflow;
    float        m_timeScale         = 60.0f;
    float        m_endSimulationTime = 0.0;

#ifndef __EMSCRIPTEN__
    char m_bathymetryFile[128]   = {};
    char m_displacementFile[128] = {};
#endif

    bool  m_playing        = false;
    float m_simulationTime = 0.0;

    Camera m_camera{m_windowSize, m_boundaryPos, m_cameraClipping};

    uint64_t m_stateFlags = BGFX_STATE_WRITE_MASK | BGFX_STATE_DEPTH_TEST_LESS;

    bool         m_showControls          = true;
    bool         m_showScenarioSelection = false;
    Vec2i        m_selectedDimensions    = {100, 100};
    ScenarioType m_selectedScenarioType  = ScenarioType::ArtificialTsunami;
    bool         m_cameraIs3D            = m_camera.getType() == Camera::Type::Perspective;
    bool         m_showStats             = m_debugFlags & BGFX_DEBUG_STATS;
    bool         m_showLines             = m_stateFlags & BGFX_STATE_PT_LINES;
    bool         m_autoScaleDataRange    = false;
    bool         m_vsyncEnabled          = m_resetFlags & BGFX_RESET_VSYNC;

    bool m_setFocusValueScale = false;

    const char* m_message = "";
  };

} // namespace App
