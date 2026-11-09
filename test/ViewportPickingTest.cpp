/**
 * @file ViewportPickingTest.cpp
 * @brief Unit tests for viewport mouse raycasting, point-in-bounds hit testing, and entity selection.
 */

#include "EditorContext.hpp"
#include "GameObject.hpp"
#include "Log.h"
#include "Scene.hpp"
#include "SimpleRenderer.hpp"
#include "SpriteRenderer.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace Elysium;

void TestCameraUnprojection() {
  EditorCamera cam;
  cam.center = {10.0f, 5.0f};
  cam.zoom = 1.0f;
  float ppm = 50.0f;

  // Center pixel (0, 0 offset from center) must unproject to camera center
  sf::Vector2f centerWorld = cam.ScreenToWorld({0.0f, 0.0f}, ppm);
  assert(std::abs(centerWorld.x - 10.0f) < 1e-4f);
  assert(std::abs(centerWorld.y - 5.0f) < 1e-4f);

  // 50 pixels right = +1.0m at zoom 1.0
  sf::Vector2f rightWorld = cam.ScreenToWorld({50.0f, 0.0f}, ppm);
  assert(std::abs(rightWorld.x - 11.0f) < 1e-4f);
  assert(std::abs(rightWorld.y - 5.0f) < 1e-4f);

  // At zoom 2.0 (zoomed in), 50 pixels = +0.5m
  cam.zoom = 2.0f;
  sf::Vector2f zoomedWorld = cam.ScreenToWorld({50.0f, 0.0f}, ppm);
  assert(std::abs(zoomedWorld.x - 10.5f) < 1e-4f);
  assert(std::abs(zoomedWorld.y - 5.0f) < 1e-4f);

  // ViewportToWorld test
  cam.center = {0.0f, 0.0f};
  cam.zoom = 1.0f;
  sf::Vector2f viewportSize = {800.0f, 600.0f};
  // Top-left corner (0, 0) should be (-400 / 50, -300 / 50) = (-8.0m, -6.0m)
  sf::Vector2f topLeftWorld = cam.ViewportToWorld({0.0f, 0.0f}, viewportSize, ppm);
  assert(std::abs(topLeftWorld.x - (-8.0f)) < 1e-4f);
  assert(std::abs(topLeftWorld.y - (-6.0f)) < 1e-4f);

  // Viewport center (400, 300) should be (0.0m, 0.0m)
  sf::Vector2f vpCenterWorld = cam.ViewportToWorld({400.0f, 300.0f}, viewportSize, ppm);
  assert(std::abs(vpCenterWorld.x - 0.0f) < 1e-4f);
  assert(std::abs(vpCenterWorld.y - 0.0f) < 1e-4f);

  std::cout << "[PASS] TestCameraUnprojection" << std::endl;
}

void TestSpritePicking() {
  Scene scene;
  SimpleRenderer renderer(50.0f);

  // 1. Box Sprite entity
  auto boxEntity = std::make_shared<GameObject>("BoxEntity");
  boxEntity->GetTransform().SetLocalPosition({5.0f, 5.0f, 0.0f});
  boxEntity->GetTransform().SetLocalScale({1.0f, 1.0f, 1.0f});
  boxEntity->AddComponent<SpriteRenderer>(sf::Color::Red, sf::Vector2f(2.0f, 2.0f)); // size 2m x 2m, centered
  scene.AddGameObject(boxEntity);

  // 2. Circle Sprite entity
  auto circleEntity = std::make_shared<GameObject>("CircleEntity");
  circleEntity->GetTransform().SetLocalPosition({10.0f, 10.0f, 0.0f});
  circleEntity->GetTransform().SetLocalScale({1.0f, 1.0f, 1.0f});
  circleEntity->AddComponent<SpriteRenderer>(sf::Color::Blue, 1.0f); // radius 1m, centered
  scene.AddGameObject(circleEntity);

  scene.UpdateTransforms();

  // Test inside box
  auto hit1 = renderer.PickEntity(scene, Vec2(5.5f, 5.5f));
  assert(hit1 == boxEntity);

  // Test corner of box (edge: 5.0 + 1.0 = 6.0)
  auto hit2 = renderer.PickEntity(scene, Vec2(5.9f, 5.9f));
  assert(hit2 == boxEntity);

  // Test just outside box
  auto miss1 = renderer.PickEntity(scene, Vec2(6.2f, 5.0f));
  assert(miss1 == nullptr);

  // Test inside circle
  auto hit3 = renderer.PickEntity(scene, Vec2(10.5f, 10.0f));
  assert(hit3 == circleEntity);

  // Test just outside circle (r = 1.0, distance = 1.2)
  auto miss2 = renderer.PickEntity(scene, Vec2(11.2f, 10.0f));
  assert(miss2 == nullptr);

  std::cout << "[PASS] TestSpritePicking" << std::endl;
}

void TestColliderPicking() {
  Scene scene;
  SimpleRenderer renderer(50.0f);

  // Physics-only entity with Box collider (extents 1.5m x 1.0m)
  auto physBox = std::make_shared<GameObject>("PhysBox");
  physBox->GetTransform().SetLocalPosition({-5.0f, 0.0f, 0.0f});
  auto &body = physBox->CreateRigidBody();
  body.AddColliders(Collider::CreateBox(Vec3(1.5f, 1.0f, 0.5f), 1.0f));
  scene.AddGameObject(physBox);

  // Physics-only entity with Sphere collider (radius 0.8m)
  auto physSphere = std::make_shared<GameObject>("PhysSphere");
  physSphere->GetTransform().SetLocalPosition({0.0f, -5.0f, 0.0f});
  auto &bodySphere = physSphere->CreateRigidBody();
  bodySphere.AddColliders(Collider::CreateSphere(0.8f, 1.0f));
  scene.AddGameObject(physSphere);

  scene.UpdateTransforms();

  // Inside physBox
  auto hit1 = renderer.PickEntity(scene, Vec2(-4.0f, 0.5f));
  assert(hit1 == physBox);

  // Outside physBox
  auto miss1 = renderer.PickEntity(scene, Vec2(-3.0f, 0.0f));
  assert(miss1 == nullptr);

  // Inside physSphere
  auto hit2 = renderer.PickEntity(scene, Vec2(0.0f, -4.5f));
  assert(hit2 == physSphere);

  // Outside physSphere
  auto miss2 = renderer.PickEntity(scene, Vec2(0.0f, -6.0f));
  assert(miss2 == nullptr);

  std::cout << "[PASS] TestColliderPicking" << std::endl;
}

void TestTopMostLayerSelection() {
  Scene scene;
  SimpleRenderer renderer(50.0f);

  // Entity A at Z = 0
  auto entityA = std::make_shared<GameObject>("EntityA");
  entityA->GetTransform().SetLocalPosition({0.0f, 0.0f, 0.0f});
  entityA->AddComponent<SpriteRenderer>(sf::Color::Green, sf::Vector2f(2.0f, 2.0f));
  scene.AddGameObject(entityA);

  // Entity B at Z = 1.0 (in front of A)
  auto entityB = std::make_shared<GameObject>("EntityB");
  entityB->GetTransform().SetLocalPosition({0.0f, 0.0f, 1.0f});
  entityB->AddComponent<SpriteRenderer>(sf::Color::Yellow, sf::Vector2f(2.0f, 2.0f));
  scene.AddGameObject(entityB);

  scene.UpdateTransforms();

  // Pick at (0, 0) overlapping both: Entity B has higher Z, so it must be selected
  auto topHit = renderer.PickEntity(scene, Vec2(0.0f, 0.0f));
  assert(topHit == entityB);

  // Change Z of Entity A to 2.0 (now A is in front of B)
  entityA->GetTransform().SetLocalPosition({0.0f, 0.0f, 2.0f});
  scene.UpdateTransforms();

  auto topHit2 = renderer.PickEntity(scene, Vec2(0.0f, 0.0f));
  assert(topHit2 == entityA);

  // Two entities at same Z: later added entity is drawn on top
  entityA->GetTransform().SetLocalPosition({0.0f, 0.0f, 0.0f});
  entityB->GetTransform().SetLocalPosition({0.0f, 0.0f, 0.0f});
  scene.UpdateTransforms();

  auto topHit3 = renderer.PickEntity(scene, Vec2(0.0f, 0.0f));
  assert(topHit3 == entityB);

  std::cout << "[PASS] TestTopMostLayerSelection" << std::endl;
}

void TestEmptySpaceDeselection() {
  Scene scene;
  SimpleRenderer renderer(50.0f);

  auto entity = std::make_shared<GameObject>("Solo");
  entity->GetTransform().SetLocalPosition({0.0f, 0.0f, 0.0f});
  entity->AddComponent<SpriteRenderer>(sf::Color::White, sf::Vector2f(1.0f, 1.0f));
  scene.AddGameObject(entity);
  scene.UpdateTransforms();

  // Click far away
  auto result = renderer.PickEntity(scene, Vec2(100.0f, 100.0f));
  assert(result == nullptr);

  // Inactive entity should not be picked
  entity->active = false;
  auto resultInactive = renderer.PickEntity(scene, Vec2(0.0f, 0.0f));
  assert(resultInactive == nullptr);

  std::cout << "[PASS] TestEmptySpaceDeselection" << std::endl;
}

int main() {
  Elysium::Log::Init();
  std::cout << "Starting ViewportPickingTest..." << std::endl;
  TestCameraUnprojection();
  TestSpritePicking();
  TestColliderPicking();
  TestTopMostLayerSelection();
  TestEmptySpaceDeselection();
  std::cout << "All ViewportPickingTest cases PASSED!" << std::endl;
  return 0;
}
