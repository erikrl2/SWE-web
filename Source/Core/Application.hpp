#include <bgfx/bgfx.h>
#include <GLFW/glfw3.h>
#include <string>

int main(int argc, char** argv);

namespace Core {

  class Application {
  public:
    Application(const std::string& title, int width, int height);

    virtual ~Application();

    virtual void update()      = 0;
    virtual void updateImGui() = 0;

  public:
    static Application* get() { return s_app; }

  protected:
    std::string m_title;
    int         m_width;
    int         m_height;

    GLFWwindow* m_window;

    const bgfx::ViewId m_mainView;

  private:
    void run();

    friend int ::main(int argc, char** argv);

  private:
    static void glfwErrorCallback(int error, const char* description);
    static void windowSizeCallback(GLFWwindow* window, int width, int height);

  private:
    static inline Application* s_app = nullptr;
  };

  Application* createApplication(int argc, char** argv);

} // namespace Core
