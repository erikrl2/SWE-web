#include "Camera.hpp"

#include <GLFW/glfw3.h>
#include <iostream>

namespace App {

  Camera::Camera(Type type, const Vec2i& windowSize, const Vec4f& boundaryPos, const Vec2f& cameraClipping):
    m_type(type),
    m_windowSize(windowSize),
    m_boundaryPos(boundaryPos),
    m_cameraClipping(cameraClipping) {}

  void Camera::update(float dt) {
    switch (m_type) {
    case Type::Orthographic:
      updateOrthographic(dt);
      break;
    case Type::Perspective:
      updatePerspective(dt);
      break;
    }
  }

  void Camera::applyViewProjection() {
    switch (m_type) {
    case Type::Orthographic:
      applyOrthographic();
      break;
    case Type::Perspective:
      applyPerspective();
      break;
    }
  }

  void Camera::onMouseScrolled(float delta) {
    m_zoom *= (1.0f - delta * 0.1f);
    m_zoom = std::max(0.1f, m_zoom);
  }

  void Camera::updateOrthographic(float) {
    bool leftButtonPressed = Core::Input::isButtonPressed(GLFW_MOUSE_BUTTON_LEFT);

    if (leftButtonPressed && !m_active && !m_mouseOverUI) {
      m_active          = true;
      m_initialMousePos = Core::Input::getMousePosition();
      return;
    } else if (!leftButtonPressed && m_active) {
      m_active = false;
    }

    if (m_active) {
      Vec2  mousePos   = Core::Input::getMousePosition();
      Vec2  mouseDelta = mousePos - m_initialMousePos;
      float scaleW     = (m_boundaryPos[1] - m_boundaryPos[0]) / (float)m_windowSize.x;
      float scaleH     = (m_boundaryPos[3] - m_boundaryPos[2]) / (float)m_windowSize.y;

      m_pan.x -= mouseDelta.x * m_zoom * scaleW;
      m_pan.y += mouseDelta.y * m_zoom * scaleH;
      m_initialMousePos = mousePos;
    }
  }

  void Camera::applyOrthographic() {
    Vec2f lowerBound = m_boundaryPos.xz() * m_zoom + m_pan;
    Vec2f upperBound = m_boundaryPos.yw() * m_zoom + m_pan;

    float domainWidth  = upperBound.x - lowerBound.x;
    float domainHeight = upperBound.y - lowerBound.y;

    float aspect = (float)m_windowSize.x / (float)m_windowSize.y;

    if (aspect > domainWidth / domainHeight) {
      float width = domainHeight * aspect;
      lowerBound.x += (domainWidth - width) * 0.5f;
      upperBound.x = lowerBound.x + width;
    } else {
      float height = domainWidth / aspect;
      lowerBound.y += (domainHeight - height) * 0.5f;
      upperBound.y = lowerBound.y + height;
    }

    float proj[16];
    bx::mtxOrtho(
      proj, lowerBound.x, upperBound.x, lowerBound.y, upperBound.y, m_cameraClipping[0], m_cameraClipping[1], 0, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Left
    );
    bgfx::setViewTransform(0, nullptr, proj);
  }

  void Camera::updatePerspective(float) {}

  void Camera::applyPerspective() {}

} // namespace App
