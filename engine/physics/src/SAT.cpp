/**
 * @file SAT.cpp
 * @brief Implementation of Separating Axis Theorem and Minimum Translation Vector (MTV).
 */

#include "elypch.hpp"
#include "SAT.hpp"
#include <cfloat>

namespace Elysium {

Projection SAT::Project(const std::vector<Vec3> &vertices, const Vec3 &axis) {
  if (vertices.empty())
    return {0.0f, 0.0f};

  float min = axis.Dot(vertices[0]);
  float max = min;

  for (size_t i = 1; i < vertices.size(); ++i) {
    float dot = axis.Dot(vertices[i]);
    if (dot < min)
      min = dot;
    if (dot > max)
      max = dot;
  }

  return {min, max};
}

std::vector<Vec3> SAT::GetAxes(const std::vector<Vec3> &vertices) {
  std::vector<Vec3> axes;
  axes.reserve(vertices.size());

  for (size_t i = 0; i < vertices.size(); ++i) {
    const Vec3 &p1 = vertices[i];
    const Vec3 &p2 = vertices[(i + 1) % vertices.size()];

    // Edge vector: e = p2 - p1
    Vec3 edge = p2 - p1;
    float lenSq = edge.MagnitudeSquared();
    if (lenSq <= EPSILON * EPSILON) {
      continue; // Skip degenerate zero-length edges
    }

    // 2D Perpendicular to edge
    Vec3 normal(-edge.y, edge.x, 0.0f);
    normal.Normalize();

    // Check if a parallel or anti-parallel axis is already present
    bool isDuplicate = false;
    for (const auto &existing : axes) {
      if (std::abs(existing.Dot(normal)) > 1.0f - 1e-4f) {
        isDuplicate = true;
        break;
      }
    }

    if (!isDuplicate) {
      axes.push_back(normal);
    }
  }

  return axes;
}

SATResult SAT::TestPolygonPolygon(const std::vector<Vec3> &verticesA,
                                  const std::vector<Vec3> &verticesB) {
  SATResult result;
  if (verticesA.size() < 3 || verticesB.size() < 3) {
    result.collided = false;
    return result;
  }

  float minOverlap = FLT_MAX;
  Vec3 smallestAxis(0.0f, 0.0f, 0.0f);

  // 1. Candidate axes from Polygon A
  std::vector<Vec3> axesA = GetAxes(verticesA);
  // 2. Candidate axes from Polygon B
  std::vector<Vec3> axesB = GetAxes(verticesB);

  if (axesA.empty() || axesB.empty()) {
    result.collided = false;
    return result;
  }
  // Iterate both axis vectors directly without allocating an allAxes vector
  for (const auto *axesPtr : {&axesA, &axesB}) {
    for (const auto &axis : *axesPtr) {
      // Project both shapes onto axis
      Projection pA = Project(verticesA, axis);
      Projection pB = Project(verticesB, axis);
      // If any separating axis is found, shapes do NOT collide
      if (!pA.Overlaps(pB)) {
        result.collided = false;
        return result;
      }
      
      float overlap = ComputeOverlap(pA, pB);

      if (overlap < minOverlap) {
        minOverlap = overlap;
        float midA = 0.5f * (pA.min + pA.max);
        float midB = 0.5f * (pB.min + pB.max);
        smallestAxis = (midA <= midB) ? axis : -axis; // Always points A -> B
      }
      if (overlap < minOverlap) {
        minOverlap = overlap;
        float midA = 0.5f * (pA.min + pA.max);
        float midB = 0.5f * (pB.min + pB.max);
        smallestAxis = (midA <= midB) ? axis : -axis; // Always points A -> B
      }
    }
  }

  // All axes overlap -> collision confirmed
  result.collided = true;
  result.depth = minOverlap;
  result.normal = smallestAxis;
  return result;
}


Projection SAT::ProjectCircle(const Vec3 &center, float radius, const Vec3 &axis) {
  float centerProj = axis.Dot(center);
  return {centerProj - radius, centerProj + radius};
}

SATResult SAT::TestPolygonCircle(const std::vector<Vec3> &polygonVertices, const Vec3 &circleCenter, float circleRadius) {
  SATResult result;
  if (polygonVertices.size() < 3) {
    result.collided = false;
    return result;
  }

  float minOverlap = FLT_MAX;
  Vec3 smallestAxis(0.0f, 0.0f, 0.0f);

  // -------------------------------------------------------------
  // 1. Test all edge normals of the polygon
  // -------------------------------------------------------------
  std::vector<Vec3> axes = GetAxes(polygonVertices);

  // -------------------------------------------------------------
  // 2. Add the axis between the circle center and the closest vertex
  // -------------------------------------------------------------
  float minDistanceSq = FLT_MAX;
  Vec3 closestVertex = polygonVertices[0];

  for (const auto &v : polygonVertices) {
    Vec3 diff = v - circleCenter;
    float distSq = diff.MagnitudeSquared();
    if (distSq < minDistanceSq) {
      minDistanceSq = distSq;
      closestVertex = v;
    }
  }

  Vec3 vertexAxis = closestVertex - circleCenter;
  float vertexAxisDist = vertexAxis.Magnitude();
  if (vertexAxisDist > EPSILON) {
    axes.push_back(vertexAxis / vertexAxisDist);
  }

  // -------------------------------------------------------------
  // 3. Test overlap on all candidate axes
  // -------------------------------------------------------------

  if (axes.empty()) return result;

  for (const auto &axis : axes) {
    Projection pPoly = Project(polygonVertices, axis);
    Projection pCircle = ProjectCircle(circleCenter, circleRadius, axis);

    // Separating axis found -> No collision
    if (!pPoly.Overlaps(pCircle)) {
      result.collided = false;
      return result;
    }

      float overlap = ComputeOverlap(pPoly, pCircle);

      if (overlap < minOverlap) {
        minOverlap = overlap;
        float midPoly = 0.5f * (pPoly.min + pPoly.max);
        float midCircle = 0.5f * (pCircle.min + pCircle.max);
        smallestAxis = (midPoly <= midCircle) ? axis : -axis; // Always points Polygon -> Circle
      }

      if (overlap < minOverlap) {
      minOverlap = overlap;
      float midPoly = 0.5f * (pPoly.min + pPoly.max);
      float midCircle = 0.5f * (pCircle.min + pCircle.max);
      smallestAxis = (midPoly <= midCircle) ? axis : -axis; // Always points Polygon -> Circle
    }
  }

  // All axes overlap -> collision confirmed
  result.collided = true;
  result.depth = minOverlap;
  result.normal = smallestAxis;
  return result;
}

SATResult SAT::TestCirclePolygon(const Vec3 &circleCenter, float circleRadius,
                                 const std::vector<Vec3> &polygonVertices) {
  SATResult res = TestPolygonCircle(polygonVertices, circleCenter, circleRadius);
  // Invert normal so it points from Circle (Shape A) to Polygon (Shape B)
  res.normal = -res.normal;
  return res;
}

float SAT::ComputeOverlap(const Projection &a, const Projection &b) {
  float overlap = a.GetOverlap(b);
  if (a.Contains(b) || b.Contains(a)) {
    float mins = std::abs(a.min - b.min);
    float maxs = std::abs(a.max - b.max);
    overlap += std::min(mins, maxs);
  }
  return overlap;
}


} // namespace Elysium