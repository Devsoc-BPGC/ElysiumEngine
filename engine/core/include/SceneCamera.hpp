#ifndef SCENECAMERA_HPP
#define SCENECAMERA_HPP

#include "CoreMath.hpp"
#include <SFML/Graphics/View.hpp>
#include <cstdint>

namespace Elysium {

/**
 * @brief Manages 2D orthographic projection settings and view matrix computation.
 */
class SceneCamera {
public:
  SceneCamera();
  ~SceneCamera() = default;

  void SetOrthographic(float size, float nearClip, float farClip);
  void SetViewportSize(uint32_t width, uint32_t height);

  float GetOrthographicSize() const noexcept { return m_OrthographicSize; }
  void SetOrthographicSize(float size);

  float GetNearClip() const noexcept { return m_OrthographicNear; }
  void SetNearClip(float nearClip);

  float GetFarClip() const noexcept { return m_OrthographicFar; }
  void SetFarClip(float farClip);

  float GetAspectRatio() const noexcept { return m_AspectRatio; }
  void SetAspectRatio(float aspectRatio);

  const Mat4 &GetProjection() const noexcept { return m_Projection; }

  /**
   * @brief Computes an SFML view matching this camera's projection and an entity's world transform.
   * @param worldPos World coordinates in meters.
   * @param worldRot World rotation quaternion.
   * @param pixelsPerMeter World-to-pixel conversion factor.
   */
  sf::View CalculateView(const Vec3 &worldPos, const Quat &worldRot, float pixelsPerMeter) const;

private:
  void RecalculateProjection();

private:
  float m_OrthographicSize = 10.0f; // World meters visible vertically
  float m_OrthographicNear = -1.0f;
  float m_OrthographicFar = 1.0f;
  float m_AspectRatio = 1.0f;
  Mat4 m_Projection = Mat4::Identity();
};

} // namespace Elysium

using SceneCamera = Elysium::SceneCamera;

#endif // SCENECAMERA_HPP
