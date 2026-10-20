#include "GameObject.hpp"
#include "Scene.hpp"
#include "SpriteRenderer.hpp"
#include <cmath>
#include <iostream>
#include <memory>

// Play mode simulates Scene::Clone(), so the copy must be fully independent of
// the scene the editor shows. Failures are counted explicitly rather than with
// assert(), which Release builds compile out.

namespace {

int g_failures = 0;

void Check(bool ok, const char *what) {
  if (!ok) {
    std::cout << "FAILED: " << what << std::endl;
    ++g_failures;
  }
}

class CounterComponent : public Component {
public:
  int value = 7;

  std::unique_ptr<Component> Clone() const override {
    return std::make_unique<CounterComponent>(*this);
  }
};

} // namespace

int main() {
  Scene scene;
  scene.physicsWorld.gravity = Vec3(0.0f, 9.81f, 0.0f);
  scene.physicsWorld.SetBoundaries(Vec3(8.0f, 6.0f, 0.0f),
                                   Vec3(5.0f, 5.0f, 0.0f), 0.25f);

  auto ball = std::make_shared<GameObject>("Ball");
  ball->position = Vec3(8.0f, 3.0f, 0.0f);
  auto &rb = ball->CreateRigidBody();
  rb.AddColliders(Collider::CreateSphere(0.5f, 1.0f));
  ball->AddComponent<SpriteRenderer>(sf::Color(240, 80, 80), 0.5f);
  ball->AddComponent<CounterComponent>();
  scene.AddGameObject(ball);

  auto marker = std::make_shared<GameObject>("Marker");
  marker->position = Vec3(2.0f, 2.0f, 0.0f);
  scene.AddGameObject(marker);

  std::unique_ptr<Scene> copy = scene.Clone();

  if (copy->objects.size() != 2) {
    std::cout << "FAILED: copy has " << copy->objects.size()
              << " objects, expected 2" << std::endl;
    return 1;
  }
  const auto ballCopy = copy->objects[0];
  const auto markerCopy = copy->objects[1];

  // Objects are new instances carrying the same data
  Check(ballCopy != ball && markerCopy != marker,
        "objects are new instances, not shared");
  Check(ballCopy->id == ball->id, "copy keeps the original id");
  Check(ballCopy->name == "Ball" && markerCopy->name == "Marker",
        "names copied");
  Check(ballCopy->rigidBody != nullptr &&
            ballCopy->rigidBody.get() != ball->rigidBody.get(),
        "rigid body is a new instance");
  Check(ballCopy->rigidBody && ballCopy->rigidBody->colliders.size() == 1,
        "colliders copied");
  Check(markerCopy->rigidBody == nullptr,
        "object without a rigid body still has none");

  // Components are copied and point at their new owner
  Check(ballCopy->components.size() == 2, "components copied");
  bool ownersOk = true;
  for (const auto &comp : ballCopy->components) {
    ownersOk = ownersOk && comp->gameObject == ballCopy.get();
  }
  Check(ownersOk, "copied components point at the copy");
  auto *spriteCopy = ballCopy->GetComponent<SpriteRenderer>();
  Check(spriteCopy && spriteCopy->isCircle &&
            spriteCopy->color == sf::Color(240, 80, 80),
        "SpriteRenderer data copied");

  // The copy's physics world simulates the copied bodies, with the same
  // settings as the original
  Check(copy->physicsWorld.rigidBodies.size() == 1 &&
            copy->physicsWorld.rigidBodies[0] == ballCopy->rigidBody.get(),
        "copy's world holds the copied body, not the original");
  Check(copy->physicsWorld.gravity.y == scene.physicsWorld.gravity.y,
        "gravity copied");
  Check(copy->physicsWorld.boundaryRotation == 0.25f,
        "boundary settings copied");

  // Simulating the copy leaves the original untouched
  const Vec3 originalPos = ball->position;
  for (int i = 0; i < 60; ++i) {
    copy->Update(1.0f / 60.0f);
  }
  Check(ball->position.x == originalPos.x && ball->position.y == originalPos.y,
        "original did not move while the copy was simulated");
  Check(ball->rigidBody->linearVelocity.y == 0.0f,
        "original velocity untouched");
  Check(std::fabs(ballCopy->position.y - originalPos.y) > 0.01f,
        "the copy did move");

  ballCopy->GetComponent<CounterComponent>()->value = 99;
  Check(ball->GetComponent<CounterComponent>()->value == 7,
        "changing a copied component leaves the original");

  // Removing an object from the copy leaves the original scene intact
  markerCopy->active = false;
  copy->Update(1.0f / 60.0f);
  Check(copy->objects.size() == 1 && scene.objects.size() == 2,
        "removing an object in the copy leaves the original scene");

  if (g_failures > 0) {
    std::cout << g_failures << " check(s) failed" << std::endl;
    return 1;
  }
  std::cout << "SceneCloneTest passed" << std::endl;
  return 0;
}
