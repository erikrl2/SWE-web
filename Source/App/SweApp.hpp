#include "Blocks/DimensionalSplitting.hpp"
#include "Core/Application.hpp"
#include "Types/Float2D.hpp"

namespace App {

  struct CellVertex {
    float x, y, z;

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
    Count
  };

  enum class ViewType { H, Hu, Hv, B, HPlusB, Count };

  class SweApp: public Core::Application {
  public:
    SweApp();

    ~SweApp() override;

    void update(float dt) override;
    void updateImGui(float dt) override;

  private:
    void loadBlock();
    void rescaleToDataRange();

    void updateTransform();
    void submitMesh();

  private:
    static void dropFileCallback(GLFWwindow* window, int count, const char** paths);

  private:
    bgfx::ProgramHandle m_program;

    bgfx::UniformHandle u_color;
    bgfx::UniformHandle u_util;

    float m_util[4]{};
    float m_color[4]          = {1.0f, 1.0f, 1.0f, 1.0f};
    float m_cameraClipping[2] = {0.0f, 10000.0f};

    float m_clearColor[4] = {0.32f, 0.34f, 0.43f, 1.0f};

    Blocks::Block*             m_block    = nullptr;
    const Scenarios::Scenario* m_scenario = nullptr;

    ScenarioType m_scenarioType = ScenarioType::None;
    int          m_dimensions[2]{};

    ViewType m_viewType = ViewType::H;

#ifndef __EMSCRIPTEN__
    char m_bathymetryFile[128]{};
    char m_displacementFile[128]{};
#endif

    bool   m_playing        = false;
    double m_simulationTime = 0.0;

    float m_timeScale = 60.0f;

    uint64_t m_stateFlags = BGFX_STATE_DEFAULT;
    uint32_t m_resetFlags = BGFX_RESET_VSYNC;
    uint32_t m_debugFlags = BGFX_DEBUG_NONE;
  };

} // namespace App
