#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include "CoreMath.hpp"
#include <vector>

enum class ColliderType { Sphere, Box, Polygon };
struct RigidBody;

class Collider {
public:
  ColliderType type;

  // Local transform relative to the RigidBody's 'position'
  Vec3 localCentroid;

  // Physical properties
  float mass;
  Mat3 localInertiaTensor;

  // Shape-specific data
  float radius;     // For Sphere/Circle
  Vec3 halfExtents; // For Box

  // 2D Spatial offset
  Vec2 offset = {0.0f, 0.0f};

  void SetOffset(const Vec2 &off) {
    offset = off;
    localCentroid.x = off.x;
    localCentroid.y = off.y;
  }

  void SyncOffsetFromCentroid() {
    offset.x = localCentroid.x;
    offset.y = localCentroid.y;
  }
  std::vector<Vec3> localVertices;  // For Polygon (Local coordinates)
  /**
   * @brief Constructor for a Circle (Sphere in 3D math terms).
   * @param r Radius of the circle.
   * @param density Mass per unit area.
   * @param offset Local offset from the body's origin.
   */
  static Collider CreateSphere(float r, float density,
                               Vec3 offset = Vec3(0, 0, 0)) {
    Collider c;
    c.type = ColliderType::Sphere;
    c.radius = r;
    c.localCentroid = offset;
    c.offset = Vec2(offset.x, offset.y);

    // 2D Circle Area-based mass: PI * r^2 * density
    c.mass = PI * (r * r) * density;

    // 2D Inertia Tensor for a disk (around Z axis): (1/2) * m * r^2
    float i = 0.5f * c.mass * (r * r);
    c.localInertiaTensor = Mat3::Diagonal(i, i, i);

    return c;
  }

  /**
   * @brief Constructor for a Box.
   * @param extents Half-widths in X, Y, Z.
   * @param density Mass per unit volume.
   * @param offset Local offset from the body's origin.
   */
  static Collider CreateBox(Vec3 extents, float density,
                            Vec3 offset = Vec3(0, 0, 0)) {
    Collider c;
    c.type = ColliderType::Box;
    c.halfExtents = extents;
    c.localCentroid = offset;
    c.offset = Vec2(offset.x, offset.y);

    // 2D Box Area: (2*ex) * (2*ey)
    c.mass = (2.0f * extents.x) * (2.0f * extents.y) * density;

    // 2D Inertia Tensor for a rectangle: (1/12) * m * (w^2 + h^2)
    float w = 2.0f * extents.x;
    float h = 2.0f * extents.y;
    float i = (1.0f / 12.0f) * c.mass * (w * w + h * h);
    c.localInertiaTensor = Mat3::Diagonal(i, i, i);

    // For AABB calculations, we still use a radius-like value or handle it
    // specifically. For now, let's use the distance to the furthest corner as a
    // "bounding radius".
    c.radius = extents.Magnitude();

    return c;
  }

  static Collider CreatePolygon(const std::vector<Vec3>& verts, float density = 1.0f, Vec3 offset = Vec3(0, 0, 0)) {
    Collider c;
    c.type = ColliderType::Polygon;
    c.localVertices = verts;
    c.localCentroid = offset;
    c.RecalculateProperties(density);
    return c;
  }

  /**
   * @brief Recalculates mass and local inertia tensor based on shape and
   * density.
   * @param density Mass density (default: 1.0f).
   */
  void RecalculateProperties(float density = 1.0f) {
    localCentroid.x = offset.x;
    localCentroid.y = offset.y;
    if (type == ColliderType::Sphere) {
      mass = PI * (radius * radius) * density;
      float i = 0.5f * mass * (radius * radius);
      localInertiaTensor = Mat3::Diagonal(i, i, i);
    } else if (type == ColliderType::Box) {
      mass = (2.0f * halfExtents.x) * (2.0f * halfExtents.y) * density;
      float w = 2.0f * halfExtents.x;
      float h = 2.0f * halfExtents.y;
      float i = (1.0f / 12.0f) * mass * (w * w + h * h);
      localInertiaTensor = Mat3::Diagonal(i, i, i);
      radius = halfExtents.Magnitude();
    } else if (type == ColliderType::Polygon) {
      float maxDistSq = 0.0f;
      for (const auto& v : localVertices) {
        float distSq = v.MagnitudeSquared();
        if (distSq > maxDistSq) maxDistSq = distSq;
      }
      radius = std::sqrt(maxDistSq);
      // Shoelace formula for polygon area
      float area = 0.0f;
      for (size_t i = 0; i < localVertices.size(); ++i) {
        const Vec3& p1 = localVertices[i];
        const Vec3& p2 = localVertices[(i + 1) % localVertices.size()];
        area += (p1.x * p2.y - p2.x * p1.y);
      }
      area = std::abs(area) * 0.5f;
      if (area <= 0.0f) area = 1.0f;
      mass = area * density;
      float i = (1.0f / 6.0f) * mass * (radius * radius);
      localInertiaTensor = Mat3::Diagonal(i, i, i);
    }
  }

  std::vector<Vec3> GetWorldVertices(const RigidBody& body) const;
};
/**
 * @struct CircleCollider2D
 * @brief High-level 2D circle collider specification.
 */
struct CircleCollider2D {
  float radius = 0.5f;
  float density = 1.0f;
  Vec2 offset = {0.0f, 0.0f};

  Collider ToCollider() const {
    return Collider::CreateSphere(radius, density, Vec3(offset.x, offset.y, 0.0f));
  }
};

/**
 * @struct BoxCollider2D
 * @brief High-level 2D box collider specification.
 */
struct BoxCollider2D {
  Vec2 size = {1.0f, 1.0f}; // full width and height
  float density = 1.0f;
  Vec2 offset = {0.0f, 0.0f};

  Collider ToCollider() const {
    return Collider::CreateBox(Vec3(size.x * 0.5f, size.y * 0.5f, 0.0f), density,
                               Vec3(offset.x, offset.y, 0.0f));
  }
};

using Collider2D = Collider;

namespace Elysium {
using Collider = ::Collider;
using Collider2D = ::Collider2D;
using CircleCollider2D = ::CircleCollider2D;
using BoxCollider2D = ::BoxCollider2D;
}

#endif
