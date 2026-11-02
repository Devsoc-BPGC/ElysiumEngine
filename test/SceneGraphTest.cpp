#include "GameObject.hpp"
#include "Scene.hpp"
#include "TransformComponent.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

void Check(bool cond, const char *msg) {
  if (!cond) {
    std::cerr << "[FAIL] " << msg << std::endl;
    std::exit(1);
  }
}

void TestParentChildRelationships() {
  Scene scene;

  auto parent = std::make_shared<GameObject>("Parent");
  auto child1 = std::make_shared<GameObject>("Child1");
  auto child2 = std::make_shared<GameObject>("Child2");
  auto grandChild = std::make_shared<GameObject>("GrandChild");

  scene.AddGameObject(parent);
  scene.AddGameObject(child1);
  scene.AddGameObject(child2);
  scene.AddGameObject(grandChild);

  // Set hierarchy: Parent -> Child1 -> GrandChild; Parent -> Child2
  child1->SetParent(parent->GetUUID());
  child2->SetParent(parent->GetUUID());
  grandChild->SetParent(child1->GetUUID());

  Check(child1->GetTransform().ParentHandle == parent->GetUUID(), "child1 parent handle should match parent");
  Check(child2->GetTransform().ParentHandle == parent->GetUUID(), "child2 parent handle should match parent");
  Check(grandChild->GetTransform().ParentHandle == child1->GetUUID(), "grandChild parent handle should match child1");

  Check(parent->GetTransform().ChildrenHandles.size() == 2, "parent should have 2 children");
  Check(child1->GetTransform().ChildrenHandles.size() == 1, "child1 should have 1 child");

  Check(parent->IsParentOf(child1->GetUUID()), "parent is parent of child1");
  Check(parent->IsParentOf(child2->GetUUID()), "parent is parent of child2");
  Check(parent->IsParentOf(grandChild->GetUUID()), "parent is parent of grandChild (indirect ancestor)");
  Check(child1->IsParentOf(grandChild->GetUUID()), "child1 is parent of grandChild");
  Check(!child2->IsParentOf(grandChild->GetUUID()), "child2 is not parent of grandChild");

  // Cycle prevention: Parent cannot be parented to GrandChild
  parent->SetParent(grandChild->GetUUID());
  Check(parent->GetTransform().ParentHandle == 0, "parent should still have no parent after attempting cycle");

  // Unparenting
  child1->SetParent(0);
  Check(child1->GetTransform().ParentHandle == 0, "child1 unparented to root");
  Check(parent->GetTransform().ChildrenHandles.size() == 1, "parent should now have 1 child");
  Check(!parent->IsParentOf(child1->GetUUID()), "parent is no longer parent of child1");
  Check(!parent->IsParentOf(grandChild->GetUUID()), "parent is no longer parent of grandChild");
  Check(child1->IsParentOf(grandChild->GetUUID()), "child1 is still parent of grandChild");

  std::cout << "[PASS] TestParentChildRelationships" << std::endl;
}

void TestWorldTransformCalculation() {
  Scene scene;

  auto parent = std::make_shared<GameObject>("Parent");
  auto child = std::make_shared<GameObject>("Child");

  scene.AddGameObject(parent);
  scene.AddGameObject(child);

  // Parent at position (10, 5, 0), rotated 90 degrees around Z
  parent->position = Vec3(10.0f, 5.0f, 0.0f);
  float angleRad = 90.0f * PI / 180.0f;
  parent->rotation = Quat(Vec3(0.0f, 0.0f, 1.0f), angleRad);
  parent->scale = Vec3(2.0f, 2.0f, 1.0f);
  parent->SyncTransform();

  // Child with local position (2, 0, 0)
  child->position = Vec3(2.0f, 0.0f, 0.0f);
  child->rotation = Quat();
  child->scale = Vec3(1.0f, 1.0f, 1.0f);
  child->SetParent(parent->GetUUID());

  // Parent scale = (2, 2, 1). Local offset (2, 0, 0) scaled becomes (4, 0, 0).
  // Rotated 90 degrees around Z, (4, 0, 0) becomes (0, 4, 0).
  // Added to parent translation (10, 5, 0), child world position should be (10, 9, 0).
  Vec3 childWorldPos = child->GetWorldPosition();
  Check(std::abs(childWorldPos.x - 10.0f) < 0.01f, "child world position X matches expected");
  Check(std::abs(childWorldPos.y - 9.0f) < 0.01f, "child world position Y matches expected");

  // Child world scale should be (2, 2, 1)
  Vec3 childWorldScale = child->GetWorldScale();
  Check(std::abs(childWorldScale.x - 2.0f) < 0.01f, "child world scale X matches expected");
  Check(std::abs(childWorldScale.y - 2.0f) < 0.01f, "child world scale Y matches expected");

  // Verify child GetLocalPosition stays (2, 0, 0)
  Check(std::abs(child->GetLocalPosition().x - 2.0f) < 0.01f, "child local position X unchanged");

  // Dynamic movement propagation: Move parent to (20, 10, 0)
  parent->position = Vec3(20.0f, 10.0f, 0.0f);
  parent->SyncTransform();

  childWorldPos = child->GetWorldPosition();
  Check(std::abs(childWorldPos.x - 20.0f) < 0.01f, "child world position X updated on parent move");
  Check(std::abs(childWorldPos.y - 14.0f) < 0.01f, "child world position Y updated on parent move");

  std::cout << "[PASS] TestWorldTransformCalculation" << std::endl;
}

void TestPhysicsWorldPropagation() {
  Scene scene;

  auto parent = std::make_shared<GameObject>("PhysicsParent");
  auto child = std::make_shared<GameObject>("PhysicsChild");

  scene.AddGameObject(parent);
  scene.AddGameObject(child);

  parent->position = Vec3(5.0f, 5.0f, 0.0f);
  parent->SyncTransform();

  child->position = Vec3(1.0f, 2.0f, 0.0f);
  child->CreateRigidBody();
  child->SetParent(parent->GetUUID());

  // Child world position is (6, 7, 0)
  Vec3 expectedWorldPos(6.0f, 7.0f, 0.0f);
  Check(std::abs(child->GetWorldPosition().x - expectedWorldPos.x) < 0.01f, "child world pos x");
  Check(std::abs(child->GetWorldPosition().y - expectedWorldPos.y) < 0.01f, "child world pos y");

  // Child rigid body position should match world position
  Check(std::abs(child->rigidBody->position.x - expectedWorldPos.x) < 0.01f, "child rb pos x");
  Check(std::abs(child->rigidBody->position.y - expectedWorldPos.y) < 0.01f, "child rb pos y");

  // Move parent to (10, 10, 0)
  parent->position = Vec3(10.0f, 10.0f, 0.0f);
  parent->SyncTransform();

  expectedWorldPos = Vec3(11.0f, 12.0f, 0.0f);
  Check(std::abs(child->GetWorldPosition().x - expectedWorldPos.x) < 0.01f, "child world pos x updated");
  Check(std::abs(child->rigidBody->position.x - expectedWorldPos.x) < 0.01f, "child rb pos x updated");

  std::cout << "[PASS] TestPhysicsWorldPropagation" << std::endl;
}

void TestRecursiveEntityDeletion() {
  Scene scene;

  auto parent = std::make_shared<GameObject>("Parent");
  auto child = std::make_shared<GameObject>("Child");
  auto grandChild = std::make_shared<GameObject>("GrandChild");

  scene.AddGameObject(parent);
  scene.AddGameObject(child);
  scene.AddGameObject(grandChild);

  child->SetParent(parent->GetUUID());
  grandChild->SetParent(child->GetUUID());

  Elysium::UUID parentUUID = parent->GetUUID();
  Elysium::UUID childUUID = child->GetUUID();
  Elysium::UUID grandChildUUID = grandChild->GetUUID();

  Check(scene.objects.size() == 3, "scene has 3 objects");

  // Destroying parent must recursively destroy child and grandchild
  bool res = scene.DestroyEntityByUUID(parentUUID);
  Check(res, "DestroyEntityByUUID succeeded");
  Check(scene.objects.empty(), "all entities destroyed");
  Check(scene.GetEntityByUUID(parentUUID) == nullptr, "parent removed from entity map");
  Check(scene.GetEntityByUUID(childUUID) == nullptr, "child removed from entity map");
  Check(scene.GetEntityByUUID(grandChildUUID) == nullptr, "grandchild removed from entity map");

  std::cout << "[PASS] TestRecursiveEntityDeletion" << std::endl;
}

#include "Log.h"

int main() {
  Elysium::Log::Init();

  TestParentChildRelationships();
  TestWorldTransformCalculation();
  TestPhysicsWorldPropagation();
  TestRecursiveEntityDeletion();

  std::cout << "All SceneGraph tests PASSED successfully!" << std::endl;
  return 0;
}
