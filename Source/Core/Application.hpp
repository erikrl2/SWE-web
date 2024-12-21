#include <bgfx/bgfx.h>
#include <bx/platform.h>
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <string>

int main(int argc, char** argv);

namespace Core {

  class Application {
  public:
    Application(const std::string& title, int width, int height);

    virtual ~Application();

    virtual void update(float dt)      = 0;
    virtual void updateImGui(float dt) = 0;

  public:
    static Application* get() { return s_app; }

  protected:
    std::string m_title;
    int         m_windowWidth;
    int         m_windowHeight;

    GLFWwindow* m_window;

    const bgfx::ViewId m_mainView;

  private:
    void run();

    friend int ::main(int argc, char** argv);

  private:
#ifdef __EMSCRIPTEN__
    static void emscriptenMainLoop();
#endif

    static void windowSizeCallback(GLFWwindow* window, int width, int height);

  private:
    static inline Application* s_app = nullptr;
  };

  Application* createApplication(int argc, char** argv);

} // namespace Core
