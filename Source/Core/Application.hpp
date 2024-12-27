#include <bgfx/bgfx.h>
#include <bx/platform.h>
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
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

    virtual void onKeyPressed(int key)                = 0;
    virtual void onFileDropped(std::string_view path) = 0;

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

    static EM_BOOL emscriptenResizeCallback(int eventType, const EmscriptenUiEvent* e, void* userData);
#endif

    static void glfwFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfwDropCallback(GLFWwindow* window, int count, const char** paths);

  private:
    static inline Application* s_app = nullptr;
  };

  Application* createApplication(int argc, char** argv);

} // namespace Core
