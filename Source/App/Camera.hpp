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
    Vec3f getTargetCenter() const { return m_targetCenter; }
    Vec3f getTargetOffset() const { return m_targetOffset; }
    float getZoom() const { return m_zoom; }

    void setType(Type type) { m_type = type; }
    void setMouseOverUI(bool mouseOverUI) { m_mouseOverUI = mouseOverUI; }
    void setTargetCenter(Vec3f center) { m_targetCenter = center; }

    void reset();
    void recenter();

    void update();
    void applyViewProjection();

    void onMouseScrolled(float delta);

  private:
    Vec3f forward();
    Vec3f right();
    Vec3f up();

    void pan(Vec2f delta);
    void rotate(Vec2f delta);
    void zoom(float deltaY);

  private:
    Type m_type = Type::Perspective;

    const Vec2i& m_windowSize;
    const Vec4f& m_boundaryPos;
    const Vec2f& m_cameraClipping;

    bool  m_draggingMouse = false;
    Vec2f m_initialMousePos;

    bool m_mouseOverUI = false;

    Vec3f m_targetCenter;
    Vec3f m_targetOffset;

    float m_zoom = 1.0f;

    bx::Quaternion m_orientation = bx::InitIdentity;

  private:
    const float MinZoom     = 0.1f;
    const float MaxZoom     = 10.0f;
    const float ZoomSpeed   = 0.005f;
    const float RotateSpeed = 0.005f;
  };

} // namespace App
