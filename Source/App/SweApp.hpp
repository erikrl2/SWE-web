#include "Core/Application.hpp"

#include "Types/Float2D.hpp"

namespace App {

  struct CellVertex {
    float x, y, z;

    static bgfx::VertexLayout layout;
    static void init();
  };

  class SweApp: public Core::Application {
  public:
    SweApp();

    ~SweApp() override;

    void update() override;
    void updateImGui() override;

    void updateDebugText();

  private:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

  private:
    bgfx::ProgramHandle m_program;

    bgfx::UniformHandle u_color;
    bgfx::UniformHandle u_util;

    float m_util[4]{};
    float m_color[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    float m_cameraClipping[2] = {0.0f, 20.0f};

    bool m_toggleDebugRender = false;

  private:
    Float2D<float> m_height; // Later handeled by DimSplittingBlock class

  };

} // namespace App
