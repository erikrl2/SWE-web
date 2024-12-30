#include "Camera.hpp"

#include <GLFW/glfw3.h>
#include <iostream>

namespace App {

  Camera::Camera(const Vec2i& windowSize, const Vec4f& boundaryPos, const Vec2f& cameraClipping):
    m_windowSize(windowSize),
    m_boundaryPos(boundaryPos),
    m_cameraClipping(cameraClipping) {}

  void Camera::reset() {
    m_target = {0, 0, 0};
    m_yaw    = 0.0f;
    m_pitch  = 0.0f;
    m_zoom   = 1.0f;
  }

  void Camera::update(float) {
    bool leftButtonPressed  = Core::Input::isButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    bool rightButtonPressed = Core::Input::isButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
    bool middleButtonPressed = Core::Input::isButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE);

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

  // TODO: Use quaternion for rotation
  void Camera::pan(const Vec2f& delta) {
    float scaleW = (m_boundaryPos.y - m_boundaryPos.x) / (float)m_windowSize.x;
    float scaleH = (m_boundaryPos.w - m_boundaryPos.z) / (float)m_windowSize.y;
    m_target -= right() * (delta.x * m_zoom * scaleW);
    m_target += up() * (delta.y * m_zoom * scaleH);
  }

  void Camera::rotate(const Vec2f& delta) {
    m_yaw -= delta.x * 0.01f;
    m_pitch += delta.y * 0.01f;
    m_pitch = bx::clamp(m_pitch, -bx::kPiHalf + 0.01f, bx::kPiHalf - 0.01f);
  }

  void Camera::zoom(float delta) {
    // TODO: Set limit depending on far clip
    m_zoom *= (1.0f - delta * 0.01f);
    m_zoom = std::max(0.1f, m_zoom);
  }

  void Camera::applyViewProjection() {
    float aspect       = (float)m_windowSize.x / (float)m_windowSize.y;
    float domainWidth  = m_boundaryPos.y - m_boundaryPos.x;
    float domainHeight = m_boundaryPos.w - m_boundaryPos.z;
    float distance     = std::max(domainWidth, domainHeight) * m_zoom; // TODO: Check together with fov
    Vec3f target       = m_target;

    // trial and error magic
    if (aspect > domainWidth / domainHeight) {
      target.x *= aspect * domainHeight / domainWidth;
      domainWidth = domainHeight * aspect;
    } else {
      target.y /= aspect * domainHeight / domainWidth;
      domainHeight = domainWidth / aspect;
      distance /= aspect;
    }

    target += m_targetCenter;
    Vec3f eye = target + forward() * distance;

    float view[16], proj[16];
    bx::mtxLookAt(view, eye, target, {0.0f, 1.0f, 0.0f}, bx::Handedness::Right);

    if (m_type == Type::Orthographic) {
      Vec2f lower = Vec2f{domainWidth, domainHeight} * -0.5f * m_zoom;
      Vec2f upper = Vec2f{domainWidth, domainHeight} * 0.5f * m_zoom;
      bx::mtxOrtho(proj, lower.x, upper.x, lower.y, upper.y, m_cameraClipping[0], m_cameraClipping[1], 0, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
    } else {
      float fov = 53.101f; // TODO: Auto adjust?
      bx::mtxProj(proj, fov, aspect, m_cameraClipping.x, m_cameraClipping.y, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
    }

    bgfx::setViewTransform(0, view, proj);
  }

  Vec3f Camera::forward() {
    if (m_type == Type::Orthographic) {
      return {0.0f, 0.0f, 1.0f};
    } else {
      return {bx::cos(m_pitch) * bx::sin(m_yaw), bx::sin(m_pitch), bx::cos(m_pitch) * bx::cos(m_yaw)};
    }
  }

  Vec3f Camera::right() {
    if (m_type == Type::Orthographic) {
      return {1.0f, 0.0f, 0.0f};
    } else {
      return {bx::cos(m_yaw), 0.0f, -bx::sin(m_yaw)};
    }
  }

  Vec3f Camera::up() {
    if (m_type == Type::Orthographic) {
      return {0.0f, 1.0f, 0.0f};
    } else {
      return forward().cross(right());
    }
  }

  void Camera::onMouseScrolled(float delta) {
    if (!m_mouseOverUI) {
      m_zoom *= (1.0f - delta * 0.1f);
      // TODO: Set limit depending on far clip
      m_zoom = std::max(0.1f, m_zoom);
    }
  }


  void Camera::setTargetCenter(const Vec3f& target) {
    m_targetCenter = target;
    reset();
  }

} // namespace App
