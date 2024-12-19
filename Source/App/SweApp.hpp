#include "Blocks/DimensionalSplitting.hpp"
#include "Core/Application.hpp"
#include "Types/Float2D.hpp"

namespace App {

  struct CellVertex {
    float x, y, z;

    static bgfx::VertexLayout layout;
    static void               init();
  };

  enum class ScenarioType { Tsunami, ArtificialTsunami, Test, Count };

  class SweApp: public Core::Application {
  public:
    SweApp();

    ~SweApp() override;

    void update() override;
    void updateImGui() override;

    void loadBlock(ScenarioType scenarioType, int nx, int ny);

  private:
    bgfx::ProgramHandle m_program;

    bgfx::UniformHandle u_color;
    bgfx::UniformHandle u_util;

    float m_util[4]{};
    float m_color[4]          = {1.0f, 1.0f, 1.0f, 1.0f};
    float m_cameraClipping[2] = {0.0f, 10000.0f};

    int m_debugFlags = BGFX_DEBUG_NONE;

  private:
    Blocks::Block* m_block;

    double m_simulationTime = 0.0;
  };

} // namespace App
