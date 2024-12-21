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

  class SweApp: public Core::Application {
  public:
    SweApp();

    ~SweApp() override;

    void update() override;
    void updateImGui() override;

    void updateTransform();
    void submitMesh();

    void loadBlock();

  private:
    static void dropFileCallback(GLFWwindow* window, int count, const char** paths);

  private:
    bgfx::ProgramHandle m_program;

    bgfx::UniformHandle u_color;
    bgfx::UniformHandle u_util;

    float m_util[4]{};
    float m_color[4]          = {1.0f, 1.0f, 1.0f, 1.0f};
    float m_cameraClipping[2] = {0.0f, 10000.0f};

  private:
    Blocks::Block*             m_block    = nullptr;
    const Scenarios::Scenario* m_scenario = nullptr;

    ScenarioType m_scenarioType = ScenarioType::None;
    int          m_dimensions[2]{};

    char m_bathymetryFile[128]{};
    char m_displacementFile[128]{};

    bool   m_playing        = false;
    double m_simulationTime = 0.0;
  };

} // namespace App
