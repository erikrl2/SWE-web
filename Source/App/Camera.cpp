#include "Camera.hpp"

#include <iostream>

#include "Core/Input.hpp"
#include "Utils.hpp"

namespace App {

  Camera::Camera(const Vec2i& windowSize, const Vec4f& boundaryPos, const Vec2f& cameraClipping):
    m_windowSize(windowSize),
    m_boundaryPos(boundaryPos),
    m_cameraClipping(cameraClipping) {}

  void Camera::reset() {
    recenter();
    m_orientation = bx::InitIdentity;
    m_zoom        = 1.0f;
  }

  void Camera::recenter() { m_targetOffset = {0, 0, 0}; }

  void Camera::update() {
    bool leftButtonPressed   = Core::Input::isButtonPressed(Core::Mouse::ButtonLeft);
    bool rightButtonPressed  = Core::Input::isButtonPressed(Core::Mouse::ButtonRight);
    bool middleButtonPressed = Core::Input::isButtonPressed(Core::Mouse::ButtonMiddle);

    bool buttonPressed = leftButtonPressed || rightButtonPressed || middleButtonPressed;

    if (buttonPressed && !m_draggingMouse && !m_mouseOverUI) {
      m_draggingMouse   = true;
      m_initialMousePos = Core::Input::getMousePosition();
      return;
    } else if (!buttonPressed && m_draggingMouse) {
      m_draggingMouse = false;
    }

    if (m_draggingMouse) {
      Vec2f mousePos    = Core::Input::getMousePosition();
      Vec2f mouseDelta  = mousePos - m_initialMousePos;
      m_initialMousePos = mousePos;

      switch (m_type) {
      case Type::Orthographic: {
        if (leftButtonPressed || middleButtonPressed) {
          pan(mouseDelta);
        } else if (rightButtonPressed) {
          zoom(mouseDelta.y);
        }
        break;
      }
      case Type::Perspective: {
        if (leftButtonPressed) {
          rotate(mouseDelta);
        } else if (middleButtonPressed) {
          pan(mouseDelta);
        } else if (rightButtonPressed) {
          zoom(mouseDelta.y);
        }
        break;
      }
      }
    }
  }

  void Camera::pan(Vec2f delta) {
    float scaleW = (m_boundaryPos.y - m_boundaryPos.x) / (float)m_windowSize.x;
    float scaleH = (m_boundaryPos.w - m_boundaryPos.z) / (float)m_windowSize.y;
    m_targetOffset -= right() * (delta.x * m_zoom * scaleW);
    m_targetOffset += up() * (delta.y * m_zoom * scaleH);
  }

  void Camera::rotate(Vec2f delta) {
    float deltaYaw   = delta.x * RotateSpeed;
    float deltaPitch = delta.y * RotateSpeed;

    auto yawRotation   = bx::fromAxisAngle(toBxVec3(up()), deltaYaw);
    auto pitchRotation = bx::fromAxisAngle(toBxVec3(right()), deltaPitch);
    m_orientation      = bx::mul(m_orientation, bx::mul(pitchRotation, yawRotation));
    m_orientation      = bx::normalize(m_orientation);
  }

  void Camera::zoom(float deltaY) {
    m_zoom *= (1.0f - deltaY * ZoomSpeed);
    m_zoom = std::max(MinZoom, m_zoom);
    m_zoom = std::min(MaxZoom, m_zoom);
  }

  void Camera::onMouseScrolled(float delta) {
    if (!m_mouseOverUI) {
      zoom(delta * 10.0f);
    }
  }

  void Camera::applyViewProjection() {
    Vec2f domainSize   = {m_boundaryPos.y - m_boundaryPos.x, m_boundaryPos.w - m_boundaryPos.z};
    float domainAspect = domainSize.x / domainSize.y;
    float windowAspect = (float)m_windowSize.x / (float)m_windowSize.y;
    float aspect       = windowAspect / domainAspect;
    Vec3f targetOffset = m_targetOffset;
    float farPlane     = m_cameraClipping.y;

    if (aspect > 1.0f) {
      domainSize.x *= aspect;
      targetOffset.x *= aspect;
    } else {
      domainSize.y /= aspect;
      targetOffset.y /= aspect;
      farPlane /= aspect; // because
    }

    float distance = domainSize.y * m_zoom;
    Vec3f target   = m_targetCenter + targetOffset;
    Vec3f eye      = target - forward() * distance;

    float view[16], proj[16];
    bx::mtxLookAt(view, toBxVec3(eye), toBxVec3(target), toBxVec3(up()), bx::Handedness::Right);

    if (m_type == Type::Orthographic) {
      Vec2f lower = domainSize * -0.5f * m_zoom;
      Vec2f upper = domainSize * 0.5f * m_zoom;
      bx::mtxOrtho(proj, lower.x, upper.x, lower.y, upper.y, -farPlane, farPlane, 0, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
    } else {
      float fov = 53.101f; // for seamless transition between ortho and perspective
      bx::mtxProj(proj, fov, windowAspect, m_cameraClipping.x, farPlane, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
    }

    bgfx::setViewTransform(0, view, proj);
  }

  Vec3f Camera::forward() {
    bx::Vec3 forwardDir = {0.0f, 0.0f, -1.0f};
    return toVec3f(m_type == Type::Orthographic ? forwardDir : bx::mul(forwardDir, m_orientation));
  }

  Vec3f Camera::right() {
    bx::Vec3 rightDir = {1.0f, 0.0f, 0.0f};
    return toVec3f(m_type == Type::Orthographic ? rightDir : bx::mul(rightDir, m_orientation));
  }

  Vec3f Camera::up() {
    bx::Vec3 upDir = {0.0f, 1.0f, 0.0f};
    return toVec3f(m_type == Type::Orthographic ? upDir : bx::mul(upDir, m_orientation));
  }

} // namespace App
