#include "SAT.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace Elysium;

void TestNonCollidingSeparatedBoxes() {
  std::vector<Vec3> boxA = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };
  std::vector<Vec3> boxB = {
    Vec3(5, 0, 0), Vec3(7, 0, 0), Vec3(7, 2, 0), Vec3(5, 2, 0)
  };

  SATResult result = SAT::TestPolygonPolygon(boxA, boxB);
  assert(!result.collided);
  std::cout << "[PASS] TestNonCollidingSeparatedBoxes" << std::endl;
}

void TestOverlappingBoxesWithMTV() {
  std::vector<Vec3> boxA = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };
  std::vector<Vec3> boxB = {
    Vec3(1.5f, 0, 0), Vec3(3.5f, 0, 0), Vec3(3.5f, 2, 0), Vec3(1.5f, 2, 0)
  };

  SATResult result = SAT::TestPolygonPolygon(boxA, boxB);
  assert(result.collided);
  assert(std::abs(result.depth - 0.5f) < 1e-4f);
  assert(result.normal.x > 0.99f);
  std::cout << "[PASS] TestOverlappingBoxesWithMTV (depth: " << result.depth << ")" << std::endl;
}

void TestTriangleVsBoxOverlap() {
  std::vector<Vec3> triangle = {
    Vec3(0.0f, 1.0f, 0.0f),
    Vec3(2.0f, 0.0f, 0.0f),
    Vec3(0.0f, -1.0f, 0.0f)
  };
  std::vector<Vec3> box = {
    Vec3(1.5f, -1.0f, 0.0f), Vec3(3.5f, -1.0f, 0.0f),
    Vec3(3.5f,  1.0f, 0.0f), Vec3(1.5f,  1.0f, 0.0f)
  };

  SATResult result = SAT::TestPolygonPolygon(triangle, box);
  assert(result.collided);
  assert(std::abs(result.depth - 0.5f) < 1e-4f);
  assert(result.normal.x > 0.99f);
  std::cout << "[PASS] TestTriangleVsBoxOverlap (depth: " << result.depth << ")" << std::endl;
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
  std::vector<Vec3> box = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };
  Vec3 circleCenter(2.5f, 1.0f, 0.0f);
  float radius = 1.0f;

  SATResult result = SAT::TestPolygonCircle(box, circleCenter, radius);
  assert(result.collided);
  assert(std::abs(result.depth - 0.5f) < 1e-4f);
  assert(result.normal.x > 0.99f);
  std::cout << "[PASS] TestOverlappingPolygonCircleWithMTV (depth: " << result.depth << ")" << std::endl;
}

void TestCircleNearPolygonCorner() {
  std::vector<Vec3> box = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };

  // Case A: Separated near corner (2,2)
  Vec3 separatedCircle(3.0f, 3.0f, 0.0f);
  SATResult resA = SAT::TestPolygonCircle(box, separatedCircle, 1.0f);
  assert(!resA.collided);

  // Case B: Overlapping near corner (2,2)
  Vec3 overlappingCircle(2.5f, 2.5f, 0.0f);
  SATResult resB = SAT::TestPolygonCircle(box, overlappingCircle, 1.0f);
  assert(resB.collided);

  std::cout << "[PASS] TestCircleNearPolygonCorner" << std::endl;
}

// 1. Inverted Normal Wrapper Test
void TestCirclePolygonInvertedNormal() {
  std::vector<Vec3> box = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };
  Vec3 circleCenter(2.5f, 1.0f, 0.0f);
  float radius = 1.0f;

  // Circle is Shape A, Box is Shape B -> Normal points Circle -> Box (-X direction)
  SATResult result = SAT::TestCirclePolygon(circleCenter, radius, box);
  assert(result.collided);
  assert(std::abs(result.depth - 0.5f) < 1e-4f);
  assert(result.normal.x < -0.99f);

  std::cout << "[PASS] TestCirclePolygonInvertedNormal" << std::endl;
}

// 2. Containment Cases
void TestContainmentCases() {
  // 2A: Small box inside large box
  std::vector<Vec3> outerBox = {
    Vec3(0, 0, 0), Vec3(6, 0, 0), Vec3(6, 6, 0), Vec3(0, 6, 0)
  };
  std::vector<Vec3> innerBox = {
    Vec3(1, 2, 0), Vec3(3, 2, 0), Vec3(3, 4, 0), Vec3(1, 4, 0)
  };
  SATResult resBox = SAT::TestPolygonPolygon(outerBox, innerBox);
  assert(resBox.collided);
  assert(std::abs(resBox.depth - 3.0f) < 1e-4f); // Distance to escape left edge
  assert(resBox.normal.x < -0.99f);              // Pushes out towards -X

  // 2B: Circle fully inside polygon
  Vec3 circleCenter(3.0f, 1.0f, 0.0f); // Nearest to bottom edge y=0
  float radius = 0.5f;
  SATResult resCircle = SAT::TestPolygonCircle(outerBox, circleCenter, radius);
  assert(resCircle.collided);
  assert(std::abs(resCircle.depth - 1.5f) < 1e-4f); // 1.0 (center) + 0.5 (radius)
  assert(resCircle.normal.y < -0.99f);              // Pushes towards -Y

  std::cout << "[PASS] TestContainmentCases" << std::endl;
}

// 3. Normal Directions (Left and Below)
void TestNormalDirectionsLeftAndBelow() {
  std::vector<Vec3> boxA = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };

  // Box B to the left of A -> Normal points -X
  std::vector<Vec3> boxLeft = {
    Vec3(-1.5f, 0, 0), Vec3(0.5f, 0, 0), Vec3(0.5f, 2, 0), Vec3(-1.5f, 2, 0)
  };
  SATResult resLeft = SAT::TestPolygonPolygon(boxA, boxLeft);
  assert(resLeft.collided);
  assert(std::abs(resLeft.depth - 0.5f) < 1e-4f);
  assert(resLeft.normal.x < -0.99f);

  // Box B below A -> Normal points -Y
  std::vector<Vec3> boxBelow = {
    Vec3(0, -1.5f, 0), Vec3(2, -1.5f, 0), Vec3(2, 0.5f, 0), Vec3(0, 0.5f, 0)
  };
  SATResult resBelow = SAT::TestPolygonPolygon(boxA, boxBelow);
  assert(resBelow.collided);
  assert(std::abs(resBelow.depth - 0.5f) < 1e-4f);
  assert(resBelow.normal.y < -0.99f);

  // Circle below Polygon -> Normal points -Y
  Vec3 circleBelow(1.0f, -0.5f, 0.0f);
  SATResult resCircleBelow = SAT::TestPolygonCircle(boxA, circleBelow, 1.0f);
  assert(resCircleBelow.collided);
  assert(std::abs(resCircleBelow.depth - 0.5f) < 1e-4f);
  assert(resCircleBelow.normal.y < -0.99f);

  std::cout << "[PASS] TestNormalDirectionsLeftAndBelow" << std::endl;
}

// 4. Degenerate Input Handling
void TestDegenerateInputs() {
  std::vector<Vec3> validBox = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };
  std::vector<Vec3> line = { Vec3(0, 0, 0), Vec3(1, 1, 0) };
  std::vector<Vec3> emptyVerts = {};
  std::vector<Vec3> coincidentPoints = { Vec3(1, 1, 0), Vec3(1, 1, 0), Vec3(1, 1, 0) };

  assert(!SAT::TestPolygonPolygon(line, validBox).collided);
  assert(!SAT::TestPolygonPolygon(validBox, emptyVerts).collided);
  assert(!SAT::TestPolygonPolygon(coincidentPoints, validBox).collided);

  assert(!SAT::TestPolygonCircle(line, Vec3(0, 0, 0), 1.0f).collided);
  assert(!SAT::TestPolygonCircle(emptyVerts, Vec3(0, 0, 0), 1.0f).collided);
  assert(!SAT::TestPolygonCircle(coincidentPoints, Vec3(1, 1, 0), 1.0f).collided);

  std::cout << "[PASS] TestDegenerateInputs" << std::endl;
}

// 5. Exact Contact (gap = 0)
void TestExactContactZeroGap() {
  std::vector<Vec3> boxA = {
    Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(2, 2, 0), Vec3(0, 2, 0)
  };
  // Box B touching exactly at right edge x = 2.0
  std::vector<Vec3> boxB = {
    Vec3(2.0f, 0, 0), Vec3(4.0f, 0, 0), Vec3(4.0f, 2, 0), Vec3(2.0f, 2, 0)
  };
  SATResult resBox = SAT::TestPolygonPolygon(boxA, boxB);
  assert(resBox.collided);
  assert(std::abs(resBox.depth) < 1e-5f);

  // Circle touching right edge x = 2.0 exactly (center = 3.0, radius = 1.0)
  Vec3 touchingCircle(3.0f, 1.0f, 0.0f);
  SATResult resCircle = SAT::TestPolygonCircle(boxA, touchingCircle, 1.0f);
  assert(resCircle.collided);
  assert(std::abs(resCircle.depth) < 1e-5f);

  std::cout << "[PASS] TestExactContactZeroGap" << std::endl;
}

int main() {
  TestNonCollidingSeparatedBoxes();
  TestOverlappingBoxesWithMTV();
  TestTriangleVsBoxOverlap();
  TestNonCollidingPolygonCircle();
  TestOverlappingPolygonCircleWithMTV();
  TestCircleNearPolygonCorner();

  // New edge-case tests requested in review
  TestCirclePolygonInvertedNormal();
  TestContainmentCases();
  TestNormalDirectionsLeftAndBelow();
  TestDegenerateInputs();
  TestExactContactZeroGap();

  std::cout << "\nAll SAT Tests passed successfully!" << std::endl;
  return 0;
}