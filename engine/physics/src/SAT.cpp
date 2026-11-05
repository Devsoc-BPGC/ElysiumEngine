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
    float lenSq = edge.MagnitudeSquared();
    if (lenSq <= EPSILON * EPSILON) {
      continue; // Skip degenerate zero-length edges safely before normalizing
    }

    // 2D Perpendicular (normal) to edge: (-edge.y, edge.x, 0)
    Vec3 normal(-edge.y, edge.x, 0.0f);
    normal.Normalize();
    axes.push_back(normal);
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

  if (allAxes.empty()) return result; 

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
      float midA = 0.5f * (pA.min + pA.max);
      float midB = 0.5f * (pB.min + pB.max);
      smallestAxis = (midA <= midB) ? axis : -axis; // Always points A -> B
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

    // Check for containment
    float overlap = pPoly.GetOverlap(pCircle);
    if (pPoly.Contains(pCircle) || pCircle.Contains(pPoly)) {
      float mins = std::abs(pPoly.min - pCircle.min);
      float maxs = std::abs(pPoly.max - pCircle.max);
      overlap += std::min(mins, maxs);
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


} // namespace Elysium