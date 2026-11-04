/**
 * @file SATTest.cpp
 * @brief Unit tests for Separating Axis Theorem (SAT) implementation.
 */

#include "SAT.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace Elysium;

void TestNonCollidingSeparatedBoxes() {
  std::vector<Vec3> boxA = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };
  std::vector<Vec3> boxB = {
    Vec3(5, 5, 0), Vec3(7, 5, 0), Vec3(7, 7, 0), Vec3(5, 7, 0)
  };

  SATResult result = SAT::TestPolygonPolygon(boxA, boxB);
  assert(!result.collided);
  std::cout << "[PASS] TestNonCollidingSeparatedBoxes" << std::endl;
}

void TestOverlappingBoxesWithMTV() {
  // Box A: [0, 2] x [0, 2]
  std::vector<Vec3> boxA = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };
  // Box B: [1.5, 3.5] x [0, 2] -> 0.5 overlap along X axis
  std::vector<Vec3> boxB = {
    Vec3(1.5f, 0, 0), Vec3(3.5f, 0, 0), Vec3(3.5f, 2, 0), Vec3(1.5f, 2, 0)
  };

  SATResult result = SAT::TestPolygonPolygon(boxA, boxB);
  assert(result.collided);
  assert(std::abs(result.depth - 0.5f) < 1e-4f);
  // Normal must point from A to B (positive X direction)
  assert(result.normal.x > 0.99f);
  std::cout << "[PASS] TestOverlappingBoxesWithMTV (depth: " << result.depth << ")" << std::endl;
}

void TestRotatedPolygonTriangleVsBox() {
  // Triangle pointing right
  std::vector<Vec3> triangle = {
    Vec3(0.0f, 1.0f, 0.0f),
    Vec3(2.0f, 0.0f, 0.0f),
    Vec3(0.0f, -1.0f, 0.0f)
  };
  // Box positioned overlapping the tip of the triangle
  std::vector<Vec3> box = {
    Vec3(1.5f, -1.0f, 0.0f), Vec3(3.5f, -1.0f, 0.0f),
    Vec3(3.5f,  1.0f, 0.0f), Vec3(1.5f,  1.0f, 0.0f)
  };

  SATResult result = SAT::TestPolygonPolygon(triangle, box);
  assert(result.collided);
  assert(result.depth > 0.0f);
  std::cout << "[PASS] TestRotatedPolygonTriangleVsBox" << std::endl;
}

void TestTouchingBoundaryEdgeContact() {
  // Edge-to-edge touching (overlap = 0)
  std::vector<Vec3> boxA = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };
  std::vector<Vec3> boxB = {
    Vec3(2.001f, 0, 0), Vec3(4.001f, 0, 0), Vec3(4.001f, 2, 0), Vec3(2.001f, 2, 0)
  };

  SATResult result = SAT::TestPolygonPolygon(boxA, boxB);
  assert(!result.collided);
  std::cout << "[PASS] TestTouchingBoundaryEdgeContact" << std::endl;
}

int main() {
  TestNonCollidingSeparatedBoxes();
  TestOverlappingBoxesWithMTV();
  TestRotatedPolygonTriangleVsBox();
  TestTouchingBoundaryEdgeContact();
  TestNonCollidingPolygonCircle();
  TestOverlappingPolygonCircleWithMTV();
  TestCircleNearPolygonCorner();
  std::cout << "\nAll SAT Tests passed successfully!" << std::endl;
  return 0;
}

void TestNonCollidingPolygonCircle() {
  std::vector<Vec3> box = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };
  Vec3 circleCenter(5.0f, 1.0f, 0.0f);
  float radius = 1.0f;

  SATResult result = SAT::TestPolygonCircle(box, circleCenter, radius);
  assert(!result.collided);
  std::cout << "[PASS] TestNonCollidingPolygonCircle" << std::endl;
}

void TestOverlappingPolygonCircleWithMTV() {
  // Box: [0, 2] x [0, 2], right edge at x = 2
  std::vector<Vec3> box = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };
  // Circle center at (2.5, 1.0) with radius 1.0 -> overlaps right edge by 0.5
  Vec3 circleCenter(2.5f, 1.0f, 0.0f);
  float radius = 1.0f;

  SATResult result = SAT::TestPolygonCircle(box, circleCenter, radius);
  assert(result.collided);
  assert(std::abs(result.depth - 0.5f) < 1e-4f);
  // Normal points from Polygon towards Circle (+X direction)
  assert(result.normal.x > 0.99f);
  std::cout << "[PASS] TestOverlappingPolygonCircleWithMTV (depth: " << result.depth << ")" << std::endl;
}

void TestCircleNearPolygonCorner() {
  // Box: [0, 2] x [0, 2], corner at (2, 2)
  std::vector<Vec3> box = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };

  // Case A: Circle diagonally separated near the corner
  // Distance from corner (2,2) to (3.0, 3.0) is sqrt(2) ≈ 1.414 > 1.0
  Vec3 separatedCircle(3.0f, 3.0f, 0.0f);
  SATResult resA = SAT::TestPolygonCircle(box, separatedCircle, 1.0f);
  assert(!resA.collided);

  // Case B: Circle overlapping the diagonal corner
  // Distance from (2,2) to (2.5, 2.5) is sqrt(0.5) ≈ 0.707 < 1.0 -> Colliding!
  Vec3 overlappingCircle(2.5f, 2.5f, 0.0f);
  SATResult resB = SAT::TestPolygonCircle(box, overlappingCircle, 1.0f);
  assert(resB.collided);

  std::cout << "[PASS] TestCircleNearPolygonCorner" << std::endl;
}