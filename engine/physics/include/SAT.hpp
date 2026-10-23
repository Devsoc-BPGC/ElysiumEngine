/**
 * @file SAT.hpp
 * @brief Separating Axis Theorem (SAT) collision detection for convex polygons.
 * Reference: https://dyn4j.org/2010/01/sat/
 */

#ifndef SAT_HPP
#define SAT_HPP

#include "CoreMath.hpp"
#include <vector>

namespace Elysium {

/**
 * @brief 1D Projection interval [min, max] along an axis.
 */
struct Projection {
  float min = 0.0f;
  float max = 0.0f;

  bool Overlaps(const Projection &other) const {
    return !(max < other.min || other.max < min);
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
   * @brief Extracts outward unit normal axes from consecutive polygon edges.
   */
  static std::vector<Vec3> GetAxes(const std::vector<Vec3> &vertices);

  /**
   * @brief Tests for intersection between two convex polygons using SAT.
   * @param verticesA Vertices of Polygon A in world space (ordered CW or CCW).
   * @param verticesB Vertices of Polygon B in world space (ordered CW or CCW).
   * @return SATResult with collision flag, penetration depth, and normal from A to B.
   */
  static SATResult TestPolygonPolygon(const std::vector<Vec3> &verticesA,
                                      const std::vector<Vec3> &verticesB);
};

} // namespace Elysium

#endif // SAT_HPP