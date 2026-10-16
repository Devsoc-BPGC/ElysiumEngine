/**
 * @file SAT.cpp
 * @brief Implementation of Separating Axis Theorem and Minimum Translation Vector (MTV).
 */

#include "SAT.hpp"
#include <cfloat>
#include <cmath>

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

    // 2D Perpendicular (normal) to edge: (-edge.y, edge.x, 0)
    Vec3 normal(-edge.y, edge.x, 0.0f);
    normal.Normalize();

    if (normal.MagnitudeSquared() > EPSILON) {
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

  std::vector<Vec3> allAxes;
  allAxes.reserve(axesA.size() + axesB.size());
  allAxes.insert(allAxes.end(), axesA.begin(), axesA.end());
  allAxes.insert(allAxes.end(), axesB.begin(), axesB.end());

  for (const auto &axis : allAxes) {
    // Project both shapes onto axis
    Projection pA = Project(verticesA, axis);
    Projection pB = Project(verticesB, axis);

    // If any separating axis is found, shapes do NOT collide
    if (!pA.Overlaps(pB)) {
      result.collided = false;
      return result;
    }

    // Check for containment (dyn4j MTV handling)
    float overlap = pA.GetOverlap(pB);
    if (pA.Contains(pB) || pB.Contains(pA)) {
      float mins = std::abs(pA.min - pB.min);
      float maxs = std::abs(pA.max - pB.max);
      if (mins < maxs) {
        overlap += mins;
      } else {
        overlap += maxs;
      }
    }

    if (overlap < minOverlap) {
      minOverlap = overlap;
      smallestAxis = axis;
    }
  }

  // All axes overlap -> collision confirmed
  result.collided = true;
  result.depth = minOverlap;

  // Compute centers to orient normal from A to B
  Vec3 centerA(0, 0, 0);
  for (const auto &v : verticesA) centerA += v;
  centerA = centerA * (1.0f / static_cast<float>(verticesA.size()));

  Vec3 centerB(0, 0, 0);
  for (const auto &v : verticesB) centerB += v;
  centerB = centerB * (1.0f / static_cast<float>(verticesB.size()));

  Vec3 dir = centerB - centerA;
  if (dir.Dot(smallestAxis) < 0.0f) {
    smallestAxis = -smallestAxis;
  }

  result.normal = smallestAxis;
  return result;
}

} // namespace Elysium