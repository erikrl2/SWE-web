#include <bgfx/bgfx.h>
#include <bx/platform.h>
#include <string>

#include "Input.hpp"
#include "Types/Vec.hpp"

struct GLFWwindow;
struct EmscriptenUiEvent;

int main(int argc, char** argv);

namespace Core {

  class Application {
  public:
    Application(const std::string& title, int width, int height);

    virtual ~Application();

    virtual void update(float dt)      = 0;
    virtual void updateImGui(float dt) = 0;

    virtual void onResize(int width, int height)      = 0;
    virtual void onKeyPressed(KeyCode key)            = 0;
    virtual void onMouseScrolled(float dx, float dy)  = 0;
    virtual void onFileDropped(std::string_view path) = 0;

    GLFWwindow* getWindow() const { return m_window; }

  public:
    static Application* get() { return s_app; }

  protected:
    std::string m_title;
    Vec2i       m_windowSize;

    GLFWwindow* m_window;

    const bgfx::ViewId m_mainView;

  private:
    void run();

    friend int ::main(int argc, char** argv);

  private:
#ifdef __EMSCRIPTEN__
    static void emscriptenMainLoop();
    static bool emscriptenResizeCallback(int eventType, const EmscriptenUiEvent* e, void* userData);
#endif

    static void glfwFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void glfwDropCallback(GLFWwindow* window, int count, const char** paths);

  private:
    static inline Application* s_app = nullptr;
  };

  Application* createApplication(int argc, char** argv);

} // namespace Core
