#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include "CoreMath.hpp"
#include <vector>
#include <cmath>
#include <cassert>

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
   std::vector<Vec3> localVertices;  // For Polygon (Local coordinates)

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

    /**
   * @brief Checks if a 2D polygon is strictly convex and non-self-intersecting.
   */
  static bool IsConvex(const std::vector<Vec3>& verts) {
    if (verts.size() < 3) return false;
    bool hasPositive = false;
    bool hasNegative = false;
    size_t n = verts.size();

    for (size_t i = 0; i < n; ++i) {
      const Vec3& a = verts[i];
      const Vec3& b = verts[(i + 1) % n];
      const Vec3& c = verts[(i + 2) % n];

      float cross = (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
      if (cross > 1e-5f) hasPositive = true;
      if (cross < -1e-5f) hasNegative = true;

      // If turns change direction, the polygon is concave
      if (hasPositive && hasNegative) return false;
    }
    return true;
  }

    /**
   * @brief Computes the 2D geometric centroid (center of mass) of a polygon.
   */
  static Vec3 ComputePolygonCentroid(const std::vector<Vec3>& verts) {
    if (verts.empty()) return Vec3(0, 0, 0);
    if (verts.size() < 3) return verts[0];

    float signedArea = 0.0f;
    Vec3 centroid(0.0f, 0.0f, 0.0f);
    size_t n = verts.size();

    for (size_t i = 0; i < n; ++i) {
      const Vec3& p0 = verts[i];
      const Vec3& p1 = verts[(i + 1) % n];
      float a = (p0.x * p1.y - p1.x * p0.y);
      signedArea += a;
      centroid.x += (p0.x + p1.x) * a;
      centroid.y += (p0.y + p1.y) * a;
    }

    signedArea *= 0.5f;
    if (std::abs(signedArea) > 1e-5f) {
      centroid.x /= (6.0f * signedArea);
      centroid.y /= (6.0f * signedArea);
      return centroid;
    }

    // Fallback to arithmetic mean if area is near zero
    Vec3 mean(0, 0, 0);
    for (const auto& v : verts) mean += v;
    return mean * (1.0f / static_cast<float>(n));
  }

  static Collider CreatePolygon(const std::vector<Vec3>& verts, float density = 1.0f, Vec3 offset = Vec3(0, 0, 0)) {
    assert(verts.size() >= 3 && "Polygon collider must have at least 3 vertices.");
    assert(IsConvex(verts) && "Polygon collider must be convex for SAT collision detection.");

    Collider c;
    c.type = ColliderType::Polygon;

    // 1. Compute the geometric centroid of the input vertices
    Vec3 polyCentroid = ComputePolygonCentroid(verts);

    // 2. Recenter vertices so (0, 0) represents the center of mass
    c.localVertices.reserve(verts.size());
    for (const auto& v : verts) {
      c.localVertices.push_back(v - polyCentroid);
    }

    // 3. Position the collider's centroid at offset + centroid
    c.localCentroid = offset + polyCentroid;
    c.offset = Vec2(c.localCentroid.x, c.localCentroid.y);

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
      size_t n = localVertices.size();
      if (n < 3) {
        mass = 0.0f;
        radius = 0.0f;
        localInertiaTensor.Zero();
        return;
      }

      float maxDistSq = 0.0f;
      float A = 0.0f;
      float I0 = 0.0f;
      Vec3 c(0.0f, 0.0f, 0.0f);

      // Single-pass computation of bounding radius, area, centroid, and origin inertia
      for (size_t k = 0; k < n; ++k) {
        const Vec3& p = localVertices[k];
        const Vec3& q = localVertices[(k + 1) % n];

        float distSq = p.MagnitudeSquared();
        if (distSq > maxDistSq) maxDistSq = distSq;

        float cr = p.x * q.y - q.x * p.y;
        A += cr;
        c = c + (p + q) * cr;
        I0 += cr * (p.Dot(p) + p.Dot(q) + q.Dot(q));
      }
      radius = std::sqrt(maxDistSq);

      A *= 0.5f;
      float absArea = std::abs(A);

      assert(absArea > 1e-5f && "Polygon collider must have a positive, non-zero area (vertices cannot be collinear or degenerate).");

      if (absArea <= 1e-5f) {
        mass = 0.0f;
        localInertiaTensor.Zero();
        return;
      }

      c = c * (1.0f / (6.0f * A));
      mass = absArea * density;

      // Parallel Axis Theorem: I_cm = I_origin - mass * |centroid|^2
      float Icm = std::abs(density * I0 / 12.0f) - mass * c.MagnitudeSquared();
      if (Icm <= 0.0f) {
        Icm = (1.0f / 6.0f) * mass * (radius * radius);
      }
      localInertiaTensor = Mat3::Diagonal(Icm, Icm, Icm);
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
