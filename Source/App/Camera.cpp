#include "Camera.hpp"

#include <GLFW/glfw3.h>
#include <iostream>

namespace App {

  Camera::Camera(const Vec2i& windowSize, const Vec4f& boundaryPos, const Vec2f& cameraClipping):
    m_windowSize(windowSize),
    m_boundaryPos(boundaryPos),
    m_cameraClipping(cameraClipping) {}

  void Camera::update(float dt) {
    bool leftButtonPressed = Core::Input::isButtonPressed(GLFW_MOUSE_BUTTON_LEFT);

    if (leftButtonPressed && !m_draggingMouse && !m_mouseOverUI) {
      m_draggingMouse   = true;
      m_initialMousePos = Core::Input::getMousePosition();
      return;
    } else if (!leftButtonPressed && m_draggingMouse) {
      m_draggingMouse = false;
    }

    if (m_draggingMouse) {
      Vec2f mousePos    = Core::Input::getMousePosition();
      Vec2f mouseDelta  = mousePos - m_initialMousePos;
      m_initialMousePos = mousePos;

      switch (m_type) {
      case Type::Orthographic: {
        float scaleW = (m_boundaryPos[1] - m_boundaryPos[0]) / (float)m_windowSize.x;
        float scaleH = (m_boundaryPos[3] - m_boundaryPos[2]) / (float)m_windowSize.y;
        m_pan.x -= mouseDelta.x * m_zoom * scaleW;
        m_pan.y += mouseDelta.y * m_zoom * scaleH;
        break;
      }
      case Type::Perspective: {
        // TODO: FIX
        // m_yaw -= mouseDelta.x * 0.1f * dt;
        // m_pitch += mouseDelta.y * 0.1f * dt;
        // m_pitch = bx::clamp(m_pitch, -bx::kPiHalf, bx::kPiHalf);
        break;
      }
      }
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
    if (m_mouseOverUI) {
      return;
    }

    switch (m_type) {
    case Type::Orthographic: {
      m_zoom *= (1.0f - delta * 0.1f);
      m_zoom = std::max(0.1f, m_zoom);
      break;
    }
    case Type::Perspective: {
      // TODO
      // m_distance *= (1.0f - delta * 0.1f);
      // m_distance = std::max(1.0f, m_distance);

      m_zoom *= (1.0f - delta * 0.1f);
      m_zoom = std::max(0.1f, m_zoom);
      break;
    }
    }
  }

  void Camera::applyOrthographic() {
    // TODO: Use view matrix from perspective camera

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

  void Camera::applyPerspective() {
    float domainHeight = m_boundaryPos.w - m_boundaryPos.z;

    bx::Vec3 eye    = {m_target.x, m_target.y, m_target.z + domainHeight * m_zoom};
    bx::Vec3 target = {m_target.x, m_target.y, m_target.z};
    bx::Vec3 up     = {0.0f, 1.0f, 0.0f};

    float fov       = 60.0f;
    float aspect    = (float)m_windowSize.x / (float)m_windowSize.y;
    float nearPlane = m_cameraClipping[0];
    float farPlane  = m_cameraClipping[1];

    float view[16], proj[16];
    bx::mtxLookAt(view, eye, target, up, bx::Handedness::Right);
    bx::mtxProj(proj, fov, aspect, nearPlane, farPlane, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
    bgfx::setViewTransform(0, view, proj);

    std::cout << "eye: " << eye.x << ", " << eye.y << ", " << eye.z << " - " << "target: " << target.x << ", " << target.y << ", " << target.z << std::endl;
  }

} // namespace App
