#include <bgfx/bgfx.h>
#include <bx/math.h>

namespace App {

  class Camera {
  public:
    Camera() = default;

    Camera(float* boundaryPos, float nearClip, float farClip, int windowWidth, int windowHeight):
      m_boundaryPos{boundaryPos[0], boundaryPos[1], boundaryPos[2], boundaryPos[3]},
      m_cameraClipping{nearClip, farClip},
      m_windowWidth(windowWidth),
      m_windowHeight(windowHeight) {
      updateProjection();
    }

    void setWindowSize(int width, int height) {
      m_windowWidth  = width;
      m_windowHeight = height;
      updateProjection();
    }

    void setBoundary(float left, float right, float bottom, float top) {
      m_boundaryPos[0] = left;
      m_boundaryPos[1] = right;
      m_boundaryPos[2] = bottom;
      m_boundaryPos[3] = top;
      updateProjection();
    }

    void setClippingPlanes(float nearClip, float farClip) {
      m_cameraClipping[0] = nearClip;
      m_cameraClipping[1] = farClip;
      updateProjection();
    }

    const float* getViewMatrix() const { return m_viewMatrix; }
    const float* getProjectionMatrix() const { return m_projMatrix; }

    void setView(const bx::Vec3& eye, const bx::Vec3& target, const bx::Vec3& up = {0.0f, 1.0f, 0.0f}) { bx::mtxLookAt(m_viewMatrix, eye, target, up, bx::Handedness::Left); }

    void applyViewProjection(bgfx::ViewId viewId) const { bgfx::setViewTransform(viewId, m_viewMatrix, m_projMatrix); }

  private:
    void updateProjection() {
      float left   = m_boundaryPos[0];
      float right  = m_boundaryPos[1];
      float bottom = m_boundaryPos[2];
      float top    = m_boundaryPos[3];

      float domainWidth  = right - left;
      float domainHeight = top - bottom;

      float aspect = (float)m_windowWidth / (float)m_windowHeight;

      if (aspect > domainWidth / domainHeight) {
        float width = domainHeight * aspect;
        left += (domainWidth - width) / 2.0f;
        right = left + width;
      } else {
        float height = domainWidth / aspect;
        bottom += (domainHeight - height) / 2.0f;
        top = bottom + height;
      }

      bx::mtxOrtho(m_projMatrix, left, right, bottom, top, m_cameraClipping[0], m_cameraClipping[1], 0, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Left);
    }

  private:
    float m_boundaryPos[4]    = {};
    float m_cameraClipping[2] = {};
    int   m_windowWidth       = 0;
    int   m_windowHeight      = 0;

    float m_viewMatrix[16] = {};
    float m_projMatrix[16] = {};
  };

} // namespace App
