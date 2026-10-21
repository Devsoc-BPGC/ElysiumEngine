#include "Collider.hpp"
#include "RigidBody.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

void TestEmptyRigidBody() {
  RigidBody rb;
  assert(rb.colliders.empty());
  assert(rb.mass == 0.0f);
  assert(rb.inverseMass == 0.0f);

  // Recalculating mass on empty body should remain safe
  rb.RecalculateMassProperties();
  assert(rb.colliders.empty());
  assert(rb.mass == 0.0f);
  assert(rb.inverseMass == 0.0f);
  assert(rb.localCentroid.x == 0.0f && rb.localCentroid.y == 0.0f &&
         rb.localCentroid.z == 0.0f);

  // Integrating an empty body should safely early-return or produce no crash
  rb.Integrate(0.016f);
  std::cout << "[PASS] TestEmptyRigidBody" << std::endl;
}

void TestColliderOffsetsAndRecalculate() {
  RigidBody rb;

  // Add two identical spheres with offsets symmetric about the origin: x = -1
  // and x = +1
  Collider col1 = Collider::CreateSphere(1.0f, 1.0f, Vec3(-1.0f, 0.0f, 0.0f));
  Collider col2 = Collider::CreateSphere(1.0f, 1.0f, Vec3(1.0f, 0.0f, 0.0f));

  rb.AddColliders(col1);
  rb.AddColliders(col2);

  assert(rb.colliders.size() == 2);
  float expectedMass = col1.mass + col2.mass;
  assert(std::abs(rb.mass - expectedMass) < 1e-4f);
  assert(std::abs(rb.inverseMass - (1.0f / expectedMass)) < 1e-4f);

  // Since col1 and col2 have equal mass and symmetric offsets (-1, 0, 0) and
  // (+1, 0, 0), the composite centroid must be (0, 0, 0).
  assert(std::abs(rb.localCentroid.x) < 1e-4f);
  assert(std::abs(rb.localCentroid.y) < 1e-4f);
  assert(std::abs(rb.localCentroid.z) < 1e-4f);

  // Modifying an offset dynamically and recalculating
  rb.colliders[1].localCentroid = Vec3(3.0f, 0.0f, 0.0f);
  rb.RecalculateMassProperties();

  // Weighted centroid: (m * -1 + m * 3) / (2m) = 1.0f
  assert(std::abs(rb.localCentroid.x - 1.0f) < 1e-4f);

  std::cout << "[PASS] TestColliderOffsetsAndRecalculate" << std::endl;
}

void TestColliderRemoval() {
  RigidBody rb;
  Collider col1 = Collider::CreateSphere(0.5f, 1.0f, Vec3(-2.0f, 0.0f, 0.0f));
  Collider col2 =
      Collider::CreateBox(Vec3(0.5f, 0.5f, 0.0f), 1.0f, Vec3(2.0f, 0.0f, 0.0f));

  rb.AddColliders(col1);
  rb.AddColliders(col2);
  assert(rb.colliders.size() == 2);

  // Remove first collider
  rb.RemoveCollider(0);
  assert(rb.colliders.size() == 1);
  assert(rb.colliders[0].type == ColliderType::Box);
  assert(std::abs(rb.mass - col2.mass) < 1e-4f);
  assert(std::abs(rb.localCentroid.x - 2.0f) < 1e-4f);

  // Clear remaining colliders
  rb.ClearColliders();
  assert(rb.colliders.empty());
  assert(rb.mass == 0.0f);
  assert(rb.inverseMass == 0.0f);
  assert(std::abs(rb.localCentroid.x) < 1e-4f);

  std::cout << "[PASS] TestColliderRemoval" << std::endl;
}

int main() {
  TestEmptyRigidBody();
  TestColliderOffsetsAndRecalculate();
  TestColliderRemoval();
  std::cout << "All RigidBody decoupling tests passed successfully!"
            << std::endl;
  return 0;
}
