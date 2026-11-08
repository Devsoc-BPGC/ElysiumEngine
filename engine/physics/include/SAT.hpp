/**
 * @file SAT.hpp
 * @brief Separating Axis Theorem (SAT) collision detection for convex polygons.
 * Reference: https://dyn4j.org/2010/01/sat/
 */

#ifndef SAT_HPP
#define SAT_HPP

#include "CoreMath.hpp"
#include <vector>
#include <algorithm>

namespace Elysium {

/**
 * @brief 1D Projection interval [min, max] along an axis.
 */
struct Projection {
  float min = 0.0f;
  float max = 0.0f;

  bool Overlaps(const Projection &other) const {
    return max >= other.min && other.max >= min;
  }

  float GetOverlap(const Projection &other) const {
    if (!Overlaps(other))
      return 0.0f;
    return std::min(max, other.max) - std::max(min, other.min);
  }

  bool Contains(const Projection &other) const {
    return min <= other.min && max >= other.max;
  }
};

/**
 * @brief Result of an SAT intersection test.
 */
struct SATResult {
  bool collided = false;
  Vec3 normal; // Collision normal (MTV axis) pointing from Shape A to Shape B
  float depth = 0.0f; // Penetration depth (MTV magnitude)
};

class SAT {
public:
  /**
   * @brief Projects a list of polygon vertices onto a given unit axis.
   */
  static Projection Project(const std::vector<Vec3> &vertices, const Vec3 &axis);

  /**
   * @brief Computes 1D overlap between two projections, accounting for full containment (dyn4j MTV handling).
   */
  static float ComputeOverlap(const Projection &a, const Projection &b);

  /**
   * @brief Extracts candidate perpendicular axes from consecutive polygon edges.
   * @note In SAT, axes represent 1D projection lines (sign is bidirectional).
   * The final MTV collision normal is explicitly oriented from Shape A to Shape B.
   */
  static std::vector<Vec3> GetAxes(const std::vector<Vec3> &vertices);

  /**
   * @brief Tests for intersection between two convex polygons using SAT.
   * @param verticesA Vertices of Polygon A in world space (ordered CW or CCW).
   * @param verticesB Vertices of Polygon B in world space (ordered CW or CCW).
   * @return SATResult with collision flag, penetration depth, and normal from A to B.
   */
  static SATResult TestPolygonPolygon(const std::vector<Vec3> &verticesA, const std::vector<Vec3> &verticesB);

   
   
  /**
   * @brief Projects a 2D circle onto a unit axis.
   */
  static Projection ProjectCircle(const Vec3 &center, float radius, const Vec3 &axis);

  /**
   * @brief Tests for intersection between a convex polygon and a circle using SAT.
   * @param polygonVertices World-space vertices of the polygon.
   * @param circleCenter World-space center of the circle.
   * @param circleRadius Radius of the circle.
   * @return SATResult with collision flag, penetration depth, and normal pointing from Polygon to Circle.
   */
  static SATResult TestPolygonCircle(const std::vector<Vec3> &polygonVertices, const Vec3 &circleCenter, float circleRadius);

  /**
   * @brief Convenience overload: Circle as Shape A, Polygon as Shape B.
   * Normal points from Circle to Polygon.
   */
  static SATResult TestCirclePolygon(const Vec3 &circleCenter, float circleRadius, const std::vector<Vec3> &polygonVertices);
};



} // namespace Elysium

#endif // SAT_HPP