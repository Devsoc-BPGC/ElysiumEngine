#include "Collider.hpp"
#include "CoreMath.hpp"
#include "GameObject.hpp"
#include "NarrowPhase.hpp"
#include "Scene.hpp"
#include "SimpleRenderer.hpp"
#include "SpriteRenderer.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

void TestColliderOffsets() {
  // Test sphere collider creation with offset
  Collider sphere = Collider::CreateSphere(1.0f, 1.0f, Vec3(1.0f, 2.0f, 0.0f));
  assert(sphere.offset.x == 1.0f && sphere.offset.y == 2.0f);
  assert(sphere.localCentroid.x == 1.0f && sphere.localCentroid.y == 2.0f);

  // Test mutating offset
  sphere.SetOffset(Vec2(-3.0f, 4.0f));
  assert(sphere.offset.x == -3.0f && sphere.offset.y == 4.0f);
  assert(sphere.localCentroid.x == -3.0f && sphere.localCentroid.y == 4.0f);

  // Test BoxCollider2D wrapper
  BoxCollider2D box2d;
  box2d.size = Vec2(4.0f, 2.0f);
  box2d.offset = Vec2(0.5f, -0.5f);
  Collider box = box2d.ToCollider();
  assert(box.type == ColliderType::Box);
  assert(std::abs(box.halfExtents.x - 2.0f) < 1e-4f);
  assert(std::abs(box.halfExtents.y - 1.0f) < 1e-4f);
  assert(box.offset.x == 0.5f && box.offset.y == -0.5f);
  assert(box.localCentroid.x == 0.5f && box.localCentroid.y == -0.5f);

  // Test CircleCollider2D wrapper
  CircleCollider2D circle2d;
  circle2d.radius = 1.5f;
  circle2d.offset = Vec2(-1.0f, 3.0f);
  Collider circle = circle2d.ToCollider();
  assert(circle.type == ColliderType::Sphere);
  assert(std::abs(circle.radius - 1.5f) < 1e-4f);
  assert(circle.offset.x == -1.0f && circle.offset.y == 3.0f);

  std::cout << "[PASS] TestColliderOffsets" << std::endl;
}

void TestNarrowphaseCollisionWithOffsets() {
  // Test collision between two bodies whose centroids are offset from entity positions
  RigidBody rbA;
  rbA.mass = 1.0f;
  rbA.inverseMass = 1.0f;
  rbA.position = Vec3(0.0f, 0.0f, 0.0f);
  // Collider centered at offset (1.0, 0.0) with radius 1.0 -> world center = (1.0, 0, 0)
  rbA.AddColliders(Collider::CreateSphere(1.0f, 1.0f, Vec3(1.0f, 0.0f, 0.0f)));
  rbA.UpdateGlobalCentroidFromPosition();

  RigidBody rbB;
  rbB.mass = 1.0f;
  rbB.inverseMass = 1.0f;
  // Entity at X = 5.0, collider with offset (-2.5, 0.0) -> world center = (2.5, 0, 0)
  rbB.position = Vec3(5.0f, 0.0f, 0.0f);
  rbB.AddColliders(Collider::CreateSphere(1.0f, 1.0f, Vec3(-2.5f, 0.0f, 0.0f)));
  rbB.UpdateGlobalCentroidFromPosition();

  // Before resolution: colA world pos = (1.0, 0, 0), colB world pos = (2.5, 0, 0)
  // Distance between centers = 1.5. Radius sum = 2.0 -> Overlap of 0.5!
  NarrowPhase::ResolveCollision(&rbA, &rbB);

  // Position correction should separate them so distance is >= radiusSum
  Vec3 posA = rbA.LocalToGlobal(rbA.colliders[0].localCentroid);
  Vec3 posB = rbB.LocalToGlobal(rbB.colliders[0].localCentroid);
  float dist = (posB - posA).Magnitude();
  assert(dist >= 1.99f);

  std::cout << "[PASS] TestNarrowphaseCollisionWithOffsets" << std::endl;
}

void TestCompoundDrawTransform() {
  // Verify SimpleRenderer can execute without crash with various offsets and pivots
  Scene scene;
  auto go = std::make_shared<GameObject>("TestEntity");
  go->position = Vec3(2.0f, 3.0f, 0.0f);
  go->rotation = Quat(Vec3(0, 0, 1), 0.5f);
  go->scale = Vec3(1.5f, 1.5f, 1.0f);

  auto &sprite = go->AddComponent<SpriteRenderer>(sf::Color::Blue, sf::Vector2f(2.0f, 2.0f));
  sprite.LocalPositionOffset = Vec2(0.5f, -0.5f);
  sprite.LocalRotationOffset = 30.0f;
  sprite.LocalScaleOffset = Vec2(0.8f, 1.2f);
  sprite.Pivot = Vec2(0.25f, 0.75f);

  auto &rb = go->CreateRigidBody();
  rb.AddColliders(Collider::CreateBox(Vec3(1.0f, 1.0f, 0.0f), 1.0f, Vec3(0.5f, -0.5f, 0.0f)));
  scene.AddGameObject(go);
  scene.physicsWorld.AddBody(&rb);

  sf::RenderTexture rt;
  if (rt.resize({800, 600})) {
    SimpleRenderer renderer(50.0f);
    renderer.drawPhysicsColliders = true;
    renderer.Render(rt, scene);
    assert(renderer.drawCalls > 0);
  }

  std::cout << "[PASS] TestCompoundDrawTransform" << std::endl;
}

#include "Log.h"

int main() {
  Elysium::Log::Init();
  TestColliderOffsets();
  TestNarrowphaseCollisionWithOffsets();
  TestCompoundDrawTransform();
  std::cout << "All Component Offsets Tests Passed Successfully!" << std::endl;
  return 0;
}
