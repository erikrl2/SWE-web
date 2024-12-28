#pragma once

#include "Types/Vec.hpp"

namespace Core {

  // TODO: Do entire input abstraction of GLFW (inc key codes)

  class Input {
  public:
    static bool  isKeyPressed(int key);
    static bool  isButtonPressed(int button);
    static Vec2f getMousePosition();
    static float getMouseX();
    static float getMouseY();
  };

} // namespace Core
