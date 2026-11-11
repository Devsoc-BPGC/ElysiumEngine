#include "EditorContext.hpp"
#include "EditorHotkeys.hpp"
#include "GameObject.hpp"
#include "Log.h"
#include "Scene.hpp"
#include <cmath>
#include <iostream>
#include <memory>

namespace {

int g_failures = 0;

void Check(bool ok, const char *what) {
  if (!ok) {
    std::cout << "FAILED: " << what << std::endl;
    ++g_failures;
  }
}

void TestParentChildTransformMath() {
  Scene scene;

  // 1. Create parent entity at (10, 20) with 45 deg rotation and (2, 2) scale
  auto parent = std::make_shared<GameObject>("Parent");
  parent->position = Vec3(10.0f, 20.0f, 0.0f);
  float angle45 = 45.0f * (3.14159265358979323846f / 180.0f);
  parent->rotation = Quat(Vec3(0, 0, 1), angle45);
  parent->scale = Vec3(2.0f, 2.0f, 1.0f);
  scene.AddGameObject(parent);

  // 2. Create child entity and parent it
  auto child = std::make_shared<GameObject>("Child");
  child->position = Vec3(0.0f, 0.0f, 0.0f);
  child->CreateRigidBody();
  scene.AddGameObject(child);
  child->SetParent(parent->GetUUID());

  // 3. User drags child with gizmo to target world transform:
  //    World position (35, 45, 0), world rotation 90 deg, world scale (3, 3, 1)
  Vec3 targetWorldPos(35.0f, 45.0f, 0.0f);
  float angle90 = 90.0f * (3.14159265358979323846f / 180.0f);
  Quat targetWorldRot(Vec3(0, 0, 1), angle90);
  Vec3 targetWorldScale(3.0f, 3.0f, 1.0f);

  Mat4 targetWorldMat = Mat4::TRS(targetWorldPos, targetWorldRot, targetWorldScale);

  // Apply parent-child inverse transformation formula:
  // M_local = M_parent,world^-1 * M_world,target
  Mat4 parentWorldInv = parent->GetWorldTransform().Inverse();
  Mat4 localMat = parentWorldInv * targetWorldMat;

  child->position = localMat.GetTranslation();
  child->rotation = localMat.GetRotation();
  child->scale = localMat.GetScale();
  child->SyncTransform();

  // Verify child world transform accurately matches the target world transform
  Vec3 resultWorldPos = child->GetWorldPosition();
  Check(std::abs(resultWorldPos.x - targetWorldPos.x) < 0.001f &&
        std::abs(resultWorldPos.y - targetWorldPos.y) < 0.001f,
        "Child world position accurately matches gizmo target world position");

  Vec3 resultWorldScale = child->GetWorldScale();
  Check(std::abs(resultWorldScale.x - targetWorldScale.x) < 0.001f &&
        std::abs(resultWorldScale.y - targetWorldScale.y) < 0.001f,
        "Child world scale accurately matches gizmo target world scale");

  // Verify attached RigidBody synchronized in real-time
  Check(child->rigidBody != nullptr, "Child has attached RigidBody");
  Check(std::abs(child->rigidBody->position.x - targetWorldPos.x) < 0.001f &&
        std::abs(child->rigidBody->position.y - targetWorldPos.y) < 0.001f,
        "Child RigidBody position is synchronized with gizmo world position");
}

void TestSnappingMath() {
  // Translation snap (0.5m step)
  float snapDist = 0.5f;
  float t1 = 1.22f;
  float t1Snapped = std::round(t1 / snapDist) * snapDist;
  Check(std::abs(t1Snapped - 1.0f) < 0.0001f, "1.22m snaps to 1.0m with 0.5m step");

  float t2 = 1.35f;
  float t2Snapped = std::round(t2 / snapDist) * snapDist;
  Check(std::abs(t2Snapped - 1.5f) < 0.0001f, "1.35m snaps to 1.5m with 0.5m step");

  float t3 = -0.8f;
  float t3Snapped = std::round(t3 / snapDist) * snapDist;
  Check(std::abs(t3Snapped - (-1.0f)) < 0.0001f, "-0.8m snaps to -1.0m with 0.5m step");

  // Rotation snap (15 deg step)
  float snapDeg = 15.0f;
  float r1 = 22.0f;
  float r1Snapped = std::round(r1 / snapDeg) * snapDeg;
  Check(std::abs(r1Snapped - 15.0f) < 0.0001f, "22 deg snaps to 15 deg with 15 deg step");

  float r2 = 27.0f;
  float r2Snapped = std::round(r2 / snapDeg) * snapDeg;
  Check(std::abs(r2Snapped - 30.0f) < 0.0001f, "27 deg snaps to 30 deg with 15 deg step");

  // Scale snap (0.1 step)
  float snapScale = 0.1f;
  float s1 = 1.04f;
  float s1Snapped = std::round(s1 / snapScale) * snapScale;
  Check(std::abs(s1Snapped - 1.0f) < 0.0001f, "1.04 snaps to 1.0 with 0.1 step");

  float s2 = 1.16f;
  float s2Snapped = std::round(s2 / snapScale) * snapScale;
  Check(std::abs(s2Snapped - 1.2f) < 0.0001f, "1.16 snaps to 1.2 with 0.1 step");
}

void TestEditorContextDefaultsAndHotkeys() {
  Elysium::EditorContext ctx;
  Check(ctx.gizmoOperation == Elysium::GizmoOperation::Translate,
        "Default gizmo operation is Translate");
  Check(ctx.gizmoMode == Elysium::GizmoMode::World,
        "Default gizmo mode is World");
  Check(ctx.snapEnabled == false,
        "Default snap is disabled");
  Check(std::abs(ctx.snapTranslation - 0.5f) < 0.0001f,
        "Default translation snap is 0.5m");
  Check(std::abs(ctx.snapRotation - 15.0f) < 0.0001f,
        "Default rotation snap is 15.0 deg");
  Check(std::abs(ctx.snapScale - 0.1f) < 0.0001f,
        "Default scale snap is 0.1");

  // Verify centralized hotkeys constants
  Check(Elysium::Hotkeys::GizmoTranslate == sf::Keyboard::Key::G,
        "GizmoTranslate hotkey is 'G'");
  Check(Elysium::Hotkeys::GizmoRotate == sf::Keyboard::Key::R,
        "GizmoRotate hotkey is 'R'");
  Check(Elysium::Hotkeys::GizmoScale == sf::Keyboard::Key::S,
        "GizmoScale hotkey is 'S'");
  Check(Elysium::Hotkeys::GizmoToggleMode == sf::Keyboard::Key::W,
        "GizmoToggleMode hotkey is 'W'");
  Check(Elysium::Hotkeys::GizmoToggleSnap == sf::Keyboard::Key::E,
        "GizmoToggleSnap hotkey is 'E'");
}

} // namespace

int main() {
  Elysium::Log::Init();

  TestParentChildTransformMath();
  TestSnappingMath();
  TestEditorContextDefaultsAndHotkeys();

  if (g_failures == 0) {
    std::cout << "All TransformGizmoTest assertions PASSED successfully." << std::endl;
    return 0;
  }

  std::cout << g_failures << " assertion(s) FAILED in TransformGizmoTest." << std::endl;
  return 1;
}
