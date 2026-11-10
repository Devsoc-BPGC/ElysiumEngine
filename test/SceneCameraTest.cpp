#include "CameraComponent.hpp"
#include "GameObject.hpp"
#include "Log.h"
#include "Scene.hpp"
#include "SceneCamera.hpp"
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

void TestOrthographicProjectionMatrix() {
  Elysium::SceneCamera cam;
  cam.SetOrthographic(10.0f, -1.0f, 1.0f);
  cam.SetAspectRatio(16.0f / 9.0f);

  const Mat4 &proj = cam.GetProjection();
  float expectedW = 10.0f * (16.0f / 9.0f);
  float expectedH = 10.0f;

  Check(std::abs(proj.m[0][0] - (2.0f / expectedW)) < 0.001f,
        "Projection m[0][0] matches 2 / width");
  Check(std::abs(proj.m[1][1] - (2.0f / expectedH)) < 0.001f,
        "Projection m[1][1] matches 2 / height");
  Check(std::abs(proj.m[2][2] - (-1.0f)) < 0.001f,
        "Projection m[2][2] matches -2 / (far - near)");
  Check(std::abs(proj.m[3][3] - 1.0f) < 0.001f,
        "Projection m[3][3] is 1.0f");

  // Center maps to origin
  Vec3 center = proj.TransformPoint(Vec3(0.0f, 0.0f, 0.0f));
  Check(std::abs(center.x) < 0.001f && std::abs(center.y) < 0.001f,
        "Center maps to (0, 0) in clip space");

  // Top-right corner maps to (1, 1)
  Vec3 topRight = proj.TransformPoint(Vec3(expectedW * 0.5f, expectedH * 0.5f, 0.0f));
  Check(std::abs(topRight.x - 1.0f) < 0.001f && std::abs(topRight.y - 1.0f) < 0.001f,
        "Top-right corner maps to (1, 1) in clip space");

  // Bottom-left corner maps to (-1, -1)
  Vec3 bottomLeft = proj.TransformPoint(Vec3(-expectedW * 0.5f, -expectedH * 0.5f, 0.0f));
  Check(std::abs(bottomLeft.x - (-1.0f)) < 0.001f && std::abs(bottomLeft.y - (-1.0f)) < 0.001f,
        "Bottom-left corner maps to (-1, -1) in clip space");
}

void TestViewportResize() {
  Elysium::SceneCamera cam;
  cam.SetViewportSize(1920, 1080);
  Check(std::abs(cam.GetAspectRatio() - (16.0f / 9.0f)) < 0.001f,
        "Viewport size 1920x1080 sets aspect ratio 16:9");

  cam.SetViewportSize(800, 600);
  Check(std::abs(cam.GetAspectRatio() - (4.0f / 3.0f)) < 0.001f,
        "Viewport size 800x600 sets aspect ratio 4:3");

  cam.SetOrthographicSize(20.0f);
  Check(std::abs(cam.GetOrthographicSize() - 20.0f) < 0.001f,
        "SetOrthographicSize updates size");
}

void TestCameraComponentCloning() {
  auto entity = std::make_shared<GameObject>("CameraEntity");
  auto &camComp = entity->AddComponent<Elysium::CameraComponent>();
  camComp.Camera.SetOrthographicSize(15.0f);
  camComp.Camera.SetNearClip(-5.0f);
  camComp.Camera.SetFarClip(5.0f);
  camComp.Primary = false;
  camComp.FixedAspectRatio = true;
  camComp.ClearColor = sf::Color(50, 100, 150, 255);

  auto clonedComp = camComp.Clone();
  auto *clonedCam = dynamic_cast<Elysium::CameraComponent *>(clonedComp.get());
  Check(clonedCam != nullptr, "Cloned component is of type CameraComponent");
  Check(std::abs(clonedCam->Camera.GetOrthographicSize() - 15.0f) < 0.001f,
        "Clone preserves OrthographicSize");
  Check(std::abs(clonedCam->Camera.GetNearClip() - (-5.0f)) < 0.001f,
        "Clone preserves NearClip");
  Check(std::abs(clonedCam->Camera.GetFarClip() - 5.0f) < 0.001f,
        "Clone preserves FarClip");
  Check(clonedCam->Primary == false, "Clone preserves Primary flag");
  Check(clonedCam->FixedAspectRatio == true, "Clone preserves FixedAspectRatio flag");
  Check(clonedCam->ClearColor == sf::Color(50, 100, 150, 255), "Clone preserves ClearColor");

  // Verify independence
  clonedCam->Camera.SetOrthographicSize(25.0f);
  Check(std::abs(camComp.Camera.GetOrthographicSize() - 15.0f) < 0.001f,
        "Modifying clone does not mutate original");
}

void TestPrimaryCameraResolution() {
  Scene scene;

  auto objA = std::make_shared<GameObject>("ObjectA");
  scene.AddGameObject(objA);

  auto camB = std::make_shared<GameObject>("CameraB");
  auto &compB = camB->AddComponent<Elysium::CameraComponent>();
  compB.Primary = false;
  scene.AddGameObject(camB);

  auto camC = std::make_shared<GameObject>("CameraC");
  auto &compC = camC->AddComponent<Elysium::CameraComponent>();
  compC.Primary = true;
  scene.AddGameObject(camC);

  Check(scene.GetPrimaryCameraEntity() == camC.get(),
        "Scene resolves CameraC as primary camera");

  // Deactivate primary camera: resolution should return nullptr
  camC->active = false;
  Check(scene.GetPrimaryCameraEntity() == nullptr,
        "Deactivated primary camera returns nullptr");

  // Promote CameraB to primary
  compB.Primary = true;
  Check(scene.GetPrimaryCameraEntity() == camB.get(),
        "Promoting CameraB resolves CameraB as primary");
}

void TestCameraFollowTransform() {
  Scene scene;

  // Create player entity at (10, 20, 0)
  auto player = std::make_shared<GameObject>("Player");
  player->position = Vec3(10.0f, 20.0f, 0.0f);
  scene.AddGameObject(player);

  // Create camera entity following player
  auto cameraEntity = std::make_shared<GameObject>("FollowCamera");
  cameraEntity->position = Vec3(10.0f, 20.0f, 0.0f);
  auto &camComp = cameraEntity->AddComponent<Elysium::CameraComponent>();
  camComp.Primary = true;
  camComp.Camera.SetOrthographicSize(10.0f);
  camComp.Camera.SetAspectRatio(1.0f);
  scene.AddGameObject(cameraEntity);

  scene.UpdateTransforms();

  Mat4 vp = scene.GetPrimaryCameraViewProjection();

  // A point in the world located exactly at the player/camera position (10, 20, 0)
  // should project to (0, 0) in clip space
  Vec3 clipPos = vp.TransformPoint(Vec3(10.0f, 20.0f, 0.0f));
  Check(std::abs(clipPos.x) < 0.001f && std::abs(clipPos.y) < 0.001f,
        "World point at camera position projects to (0, 0) in clip space");

  // Move player and camera to (30, 40, 0)
  cameraEntity->position = Vec3(30.0f, 40.0f, 0.0f);
  scene.UpdateTransforms();

  vp = scene.GetPrimaryCameraViewProjection();
  clipPos = vp.TransformPoint(Vec3(30.0f, 40.0f, 0.0f));
  Check(std::abs(clipPos.x) < 0.001f && std::abs(clipPos.y) < 0.001f,
        "Updated camera position dynamically tracks target entity");
}

void TestViewportResizeSync() {
  Scene scene;

  auto camFree = std::make_shared<GameObject>("FreeCam");
  auto &compFree = camFree->AddComponent<Elysium::CameraComponent>();
  compFree.FixedAspectRatio = false;
  scene.AddGameObject(camFree);

  auto camFixed = std::make_shared<GameObject>("FixedCam");
  auto &compFixed = camFixed->AddComponent<Elysium::CameraComponent>();
  compFixed.FixedAspectRatio = true;
  compFixed.Camera.SetAspectRatio(2.0f);
  scene.AddGameObject(camFixed);

  // Resize viewport to 16:9
  scene.OnViewportResize(1920, 1080);
  Check(std::abs(compFree.Camera.GetAspectRatio() - (16.0f / 9.0f)) < 0.001f,
        "Free aspect camera adapts to viewport resize");
  Check(std::abs(compFixed.Camera.GetAspectRatio() - 2.0f) < 0.001f,
        "Fixed aspect camera retains its custom aspect ratio");
}

void TestCalculateView() {
  Elysium::SceneCamera cam;
  cam.SetOrthographicSize(8.0f);
  cam.SetAspectRatio(1.5f);

  float ptm = 50.0f;
  Vec3 pos(4.0f, 6.0f, 0.0f);
  Quat rot(Vec3(0, 0, 1), 0.0f);

  sf::View view = cam.CalculateView(pos, rot, ptm);

  Check(std::abs(view.getCenter().x - (4.0f * ptm)) < 0.001f &&
        std::abs(view.getCenter().y - (6.0f * ptm)) < 0.001f,
        "CalculateView sets correct view center in pixels");

  float expectedW = 8.0f * 1.5f * ptm;
  float expectedH = 8.0f * ptm;
  Check(std::abs(view.getSize().x - expectedW) < 0.001f &&
        std::abs(view.getSize().y - expectedH) < 0.001f,
        "CalculateView sets correct view size in pixels");
}

void TestGamePreviewCameraResolution() {
  Scene scene;

  // Add a primary camera entity with custom position and clear color
  auto primaryCam = std::make_shared<GameObject>("MainCamera");
  primaryCam->position = Vec3(25.0f, 35.0f, 0.0f);
  auto &camComp = primaryCam->AddComponent<Elysium::CameraComponent>();
  camComp.Primary = true;
  camComp.Camera.SetOrthographicSize(12.0f);
  camComp.Camera.SetAspectRatio(16.0f / 9.0f);
  camComp.FixedAspectRatio = true;
  camComp.ClearColor = sf::Color(10, 20, 30, 255);
  scene.AddGameObject(primaryCam);

  // Add a non-primary secondary camera
  auto secCam = std::make_shared<GameObject>("SecondaryCamera");
  secCam->position = Vec3(5.0f, 5.0f, 0.0f);
  auto &secComp = secCam->AddComponent<Elysium::CameraComponent>();
  secComp.Primary = false;
  scene.AddGameObject(secCam);

  // Simulate entering Play mode by cloning the scene
  std::unique_ptr<Scene> runtimeScene = scene.Clone();
  Check(runtimeScene != nullptr, "Scene::Clone() creates runtimeScene");

  GameObject *clonedPrimary = runtimeScene->GetPrimaryCameraEntity();
  Check(clonedPrimary != nullptr, "Runtime scene resolves primary camera entity");
  Check(clonedPrimary->name == "MainCamera", "Runtime primary camera name matches MainCamera");

  auto *clonedComp = clonedPrimary->GetComponent<Elysium::CameraComponent>();
  Check(clonedComp != nullptr, "Runtime primary camera has CameraComponent");
  Check(clonedComp->Primary == true, "Runtime primary camera has Primary == true");
  Check(clonedComp->FixedAspectRatio == true, "Runtime primary camera has FixedAspectRatio == true");
  Check(clonedComp->ClearColor == sf::Color(10, 20, 30, 255), "Runtime primary camera preserves ClearColor");

  // Verify CalculateView accurately tracks runtime camera entity world transform
  float ptm = 50.0f;
  sf::View view = clonedComp->Camera.CalculateView(
      clonedPrimary->GetWorldPosition(), clonedPrimary->GetWorldRotation(), ptm);

  Check(std::abs(view.getCenter().x - (25.0f * ptm)) < 0.001f &&
        std::abs(view.getCenter().y - (35.0f * ptm)) < 0.001f,
        "GamePreview view center matches cloned primary camera world position");

  float expectedH = 12.0f * ptm;
  float expectedW = 12.0f * (16.0f / 9.0f) * ptm;
  Check(std::abs(view.getSize().x - expectedW) < 0.001f &&
        std::abs(view.getSize().y - expectedH) < 0.001f,
        "GamePreview view size matches camera orthographic size and aspect ratio");

  // Fallback test: scene without primary camera returns nullptr
  Scene emptyScene;
  Check(emptyScene.GetPrimaryCameraEntity() == nullptr,
        "Scene without camera entities returns nullptr for primary camera");
}

} // namespace

int main() {
  Elysium::Log::Init();

  TestOrthographicProjectionMatrix();
  TestViewportResize();
  TestCameraComponentCloning();
  TestPrimaryCameraResolution();
  TestCameraFollowTransform();
  TestViewportResizeSync();
  TestCalculateView();
  TestGamePreviewCameraResolution();

  if (g_failures == 0) {
    std::cout << "All SceneCameraTest assertions PASSED successfully." << std::endl;
    return 0;
  }

  std::cout << g_failures << " assertion(s) FAILED in SceneCameraTest." << std::endl;
  return 1;
}
