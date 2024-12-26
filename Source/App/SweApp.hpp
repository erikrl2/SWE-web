#include <bgfx/embedded_shader.h>
#include <vector>

#include "Blocks/DimensionalSplitting.hpp"
#include "Core/Application.hpp"
#include "Types/Float2D.hpp"

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
  enum class UtilIndex { Min, Max, ValueScale };

  class SweApp: public Core::Application {
  public:
    SweApp();

    ~SweApp() override;

    void update(float dt) override;
    void updateImGui(float dt) override;

    void onKeyPressed(int key) override;
    void onFileDropped(std::string_view path) override;

  private:
    void loadScenario();
    void resetScenario();
    void loadBlock();
    void setBlockBoundaryType();
    void rescaleToDataRange();

    void updateTransform();
    void updateGrid();

  private:
    static std::string scenarioTypeToString(ScenarioType type);
    static std::string viewTypeToString(ViewType type);
    static std::string boundaryTypeToString(BoundaryType type);
    static uint32_t    colorToInt(float* color4);

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

    float m_gridData[4]{};
    float m_boundaryPos[4]{};
    float m_util[4]{};
    float m_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    std::vector<float> m_heightMapData;

    float m_cameraClipping[2] = {0.0f, 10000.0f};
    float m_clearColor[4]     = {0.16f, 0.17f, 0.19f, 1.0f};

    Blocks::Block*             m_block    = nullptr;
    const Scenarios::Scenario* m_scenario = nullptr;

    ScenarioType m_scenarioType = ScenarioType::None;
    int          m_dimensions[2]{};

    ViewType     m_viewType          = ViewType::H;
    BoundaryType m_boundaryType      = BoundaryType::Outflow;
    float        m_timeScale         = 60.0f;
    float        m_endSimulationTime = 0.0;

#ifndef __EMSCRIPTEN__
    char m_bathymetryFile[128]{};
    char m_displacementFile[128]{};
#endif

    bool  m_playing        = false;
    float m_simulationTime = 0.0;

    uint64_t m_stateFlags = BGFX_STATE_WRITE_MASK | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CCW | BGFX_STATE_MSAA | BGFX_STATE_PT_TRISTRIP;
    uint32_t m_resetFlags = BGFX_RESET_VSYNC;
    uint32_t m_debugFlags = BGFX_DEBUG_NONE;

    bool         m_showControls          = true;
    bool         m_showScenarioSelection = false;
    int          m_selectedDimensions[2] = {100, 100};
    ScenarioType m_selectedScenarioType  = ScenarioType::ArtificialTsunami;
    bool         m_showStats             = m_debugFlags & BGFX_DEBUG_STATS;
    bool         m_showLines             = m_stateFlags & BGFX_STATE_PT_LINES;

  private:
    static const bgfx::EmbeddedShader shaders[];
  };

} // namespace App
