#include "Core/Application.hpp"

namespace App {

  class SweApp: public Core::Application {
  public:
    SweApp();

    ~SweApp() override;

    void update() override;
    void updateImGui() override;

  private:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    // static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    // static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    // static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    // static void dropFileCallback(GLFWwindow* window, int count, const char** paths);

  private:
    bgfx::VertexBufferHandle m_vertexBuffer;
    bgfx::IndexBufferHandle  m_indexBuffer;
    bgfx::ProgramHandle      m_program;

    bool m_showStats = false;
  };

} // namespace App
