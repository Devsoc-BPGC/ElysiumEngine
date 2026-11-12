#include "elypch.hpp"
#include "SceneCamera.hpp"

namespace Elysium {

SceneCamera::SceneCamera() {
  RecalculateProjection();
}

void SceneCamera::SetOrthographic(float size, float nearClip, float farClip) {
  m_OrthographicSize = size;
  m_OrthographicNear = nearClip;
  m_OrthographicFar = farClip;
  RecalculateProjection();
}

void SceneCamera::SetViewportSize(uint32_t width, uint32_t height) {
  if (width > 0 && height > 0) {
    m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
    RecalculateProjection();
  }
}

void SceneCamera::SetOrthographicSize(float size) {
  m_OrthographicSize = size;
  RecalculateProjection();
}

void SceneCamera::SetNearClip(float nearClip) {
  m_OrthographicNear = nearClip;
  RecalculateProjection();
}

void SceneCamera::SetFarClip(float farClip) {
  m_OrthographicFar = farClip;
  RecalculateProjection();
}

void SceneCamera::SetAspectRatio(float aspectRatio) {
  m_AspectRatio = aspectRatio;
  RecalculateProjection();
}

sf::View SceneCamera::CalculateView(const Vec3 &worldPos, const Quat &worldRot,
                                    float pixelsPerMeter) const {
  sf::View view;
  float viewWidth = m_OrthographicSize * m_AspectRatio * pixelsPerMeter;
  float viewHeight = m_OrthographicSize * pixelsPerMeter;
  view.setSize({viewWidth, viewHeight});
  view.setCenter({worldPos.x * pixelsPerMeter, worldPos.y * pixelsPerMeter});
  float angleDeg = 2.0f * std::atan2(worldRot.z, worldRot.w) * 180.0f / PI;
  view.setRotation(sf::degrees(angleDeg));
  return view;
}

void SceneCamera::RecalculateProjection() {
  float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
  float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
  float orthoBottom = -m_OrthographicSize * 0.5f;
  float orthoTop = m_OrthographicSize * 0.5f;

  m_Projection = Mat4::Ortho(orthoLeft, orthoRight, orthoBottom, orthoTop,
                             m_OrthographicNear, m_OrthographicFar);
}

} // namespace Elysium
