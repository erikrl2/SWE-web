#pragma once

#include <bgfx/bgfx.h>
#include <bx/math.h>

#include "Core/Input.hpp"

namespace App {

  class Camera {
  public:
    enum class Type { Orthographic, Perspective };

    Camera(Type type, const Vec2i& windowSize, const Vec4f& boundaryPos, const Vec2f& cameraClipping);

    void switchType(Type type) { m_type = type; }

    void setMouseOverUI(bool mouseOverUI) { m_mouseOverUI = mouseOverUI; }

    void update(float dt);
    void applyViewProjection();

    void onMouseScrolled(float delta);

  private:
    void updateOrthographic(float dt);
    void applyOrthographic();

    void updatePerspective(float dt);
    void applyPerspective();

  private:
    Type m_type;

    const Vec2i& m_windowSize;
    const Vec4f& m_boundaryPos;
    const Vec2f& m_cameraClipping;

    // Orthographic camera state
    float m_zoom = 1.0f;
    Vec2f m_pan;

    // Perspective camera state
    float m_pitch    = 0.0f;
    float m_yaw      = 0.0f;
    float m_distance = 5.0f;

    bool  m_active = false;
    Vec2f m_initialMousePos;

    bool m_mouseOverUI = false;
  };

} // namespace App
