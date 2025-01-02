#include "Input.hpp"

#include <GLFW/glfw3.h>

#include "Application.hpp"

namespace Core {

  bool Input::isKeyPressed(KeyCode key) {
    auto* window = Application::get()->getWindow();
    return glfwGetKey(window, key) == GLFW_PRESS;
  }

  bool Input::isButtonPressed(MouseCode button) {
    auto* window = Application::get()->getWindow();
    return glfwGetMouseButton(window, button) == GLFW_PRESS;
  }

  Vec2f Input::getMousePosition() {
    auto*  window = Application::get()->getWindow();
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    return {(float)xpos, (float)ypos};
  }

  float Input::getMouseX() { return getMousePosition().x; }

  float Input::getMouseY() { return getMousePosition().y; }

} // namespace Core
