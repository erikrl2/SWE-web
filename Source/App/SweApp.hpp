#include <bgfx/embedded_shader.h>
#include <vector>

#include "Blocks/DimensionalSplitting.hpp"
#include "Camera.hpp"
#include "Core/Application.hpp"
#include "Types/Float2D.hpp"
#include "Types/Vec.hpp"

namespace App {

  struct CellVertex {
    uint8_t placeholder = 0;

    static bgfx::VertexLayout layout;
    static void               init();
  };

  enum class ScenarioType {
    None,
#ifndef __EMSCRIPTEN__
    Tsunami, // Requires NetCDF which isn't available with Emscripten
#endif
    ArtificialTsunami,
    Test,
    // TODO: Add more scenarios
    Count
  };

  enum class ViewType { H, Hu, Hv, B, HPlusB, Count };

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
    void invalidateBlock();
    void initializeBlock();
    void loadScenario();
    void resetSimulation();
    void setUtilDataRange();
    void setCameraTargetCenter();
    void switchView(ViewType viewType);
    void switchBoundary(BoundaryType boundaryType);

    void simulate(float dt);
    void updateGrid(bool updateTexture = true);
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
    bgfx::UniformHandle u_color;

    bgfx::UniformHandle u_heightMap;
    bgfx::TextureHandle m_heightMap;

    std::vector<CellVertex> m_vertices;
    std::vector<uint32_t>   m_indices;

    Vec4f m_gridData;    // x: nx, y: ny, z: dx, w: dy
    Vec4f m_boundaryPos; // x: left, y: right, z: bottom, w: top
    Vec2f m_minMax;      // x: minVal, y: maxVal
    Vec4f m_util;        // x: colorMinVal, y: colorMaxVal, z: valueScale
    Vec4f m_color = {1.0f, 1.0f, 1.0f, 1.0f};

    std::vector<float> m_heightMapData;

    Vec2f m_cameraClipping = {0.1f, 1000.0f};
    Vec4f m_clearColor     = {0.16f, 0.17f, 0.19f, 1.0f};

    Blocks::Block*             m_block    = nullptr;
    const Scenarios::Scenario* m_scenario = nullptr;

    ScenarioType m_scenarioType = ScenarioType::None;
    Vec2i        m_dimensions;

    ViewType     m_viewType          = ViewType::H;
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

    uint64_t m_stateFlags = BGFX_STATE_WRITE_MASK | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA | BGFX_STATE_PT_TRISTRIP;
    uint32_t m_resetFlags = BGFX_RESET_VSYNC;
    uint32_t m_debugFlags = BGFX_DEBUG_NONE;

    bool         m_showControls          = true;
    bool         m_showScenarioSelection = false;
    Vec2i        m_selectedDimensions    = {100, 100};
    ScenarioType m_selectedScenarioType  = ScenarioType::ArtificialTsunami;
    bool         m_cameraIs3D            = m_camera.getType() == Camera::Type::Perspective;
    bool         m_showStats             = m_debugFlags & BGFX_DEBUG_STATS;
    bool         m_showLines             = m_stateFlags & BGFX_STATE_PT_LINES;
    bool         m_autoScaleDataRange    = false;

  private:
    static const bgfx::EmbeddedShader shaders[];
  };

} // namespace App
