#pragma once

#include <bgfx/bgfx.h>
#include <bx/math.h>

#include "Core/Input.hpp"

namespace App {

  class Camera {
  public:
    enum class Type { Orthographic, Perspective };

    Camera(const Vec2i& windowSize, const Vec4f& boundaryPos, const Vec2f& cameraClipping);

    Type  getType() const { return m_type; }
    Vec3f getTarget() const { return m_target; }

    void setType(Type type) { m_type = type; }
    void setMouseOverUI(bool mouseOverUI) { m_mouseOverUI = mouseOverUI; }
    void setTargetCenter(const Vec3f& target);
    void reset();

    void update(float dt);
    void applyViewProjection();

    void onMouseScrolled(float delta);

  private:
    Vec3f forward();
    Vec3f right();
    Vec3f up();

    void pan(const Vec2f& delta);
    void rotate(const Vec2f& delta);
    void zoom(float delta);

  private:
    Type m_type = Type::Perspective;

    const Vec2i& m_windowSize;
    const Vec4f& m_boundaryPos;
    const Vec2f& m_cameraClipping;

    bool  m_draggingMouse = false;
    Vec2f m_initialMousePos;

    bool m_mouseOverUI = false;

    Vec3f m_targetCenter;
    Vec3f m_target;
    float m_pitch = 0.0f;
    float m_yaw   = 0.0f;
    float m_zoom  = 1.0f;
  };

} // namespace App
