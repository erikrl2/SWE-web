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

    Float2D<float> m_height;

    bool m_toggleDebugRender = false;
  };

} // namespace App
