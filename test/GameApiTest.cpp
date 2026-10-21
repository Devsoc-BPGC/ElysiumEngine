#include "Game.hpp"
#include "Log.h"

#include <iostream>

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::cerr << "GameApiTest failed at line " << __LINE__ << ": "          \
                << #condition << '\n';                                        \
      return 1;                                                                \
    }                                                                          \
  } while (false)

int main() {
  // GameObject logs during construction, so initialize the engine logger just
  // as the application entry point does before creating entities.
  Elysium::Log::Init();

  Elysium::Game::Entity invalid;
  Vec3 position;
  CHECK(!invalid.IsValid());
  CHECK(!invalid.GetPosition(position));
  CHECK(invalid.GetComponent<SpriteRenderer>() == nullptr);
  CHECK(invalid.CreateRigidBody() == nullptr);
  CHECK(!invalid.ApplyForce(Vec3(1, 0, 0), Vec3()));

  ::Scene engineScene;
  Elysium::Game::Scene scene(engineScene);
  auto entity = scene.CreateEntity("test entity");
  CHECK(entity.IsValid());
  CHECK(entity.SetPosition(Vec3(2, 3, 4)));
  CHECK(entity.GetPosition(position));
  CHECK(position.x == 2 && position.y == 3 && position.z == 4);
  CHECK(!entity.HasComponent<SpriteRenderer>());
  CHECK(entity.AddComponent<SpriteRenderer>() != nullptr);
  CHECK(entity.HasComponent<SpriteRenderer>());
  CHECK(entity.GetComponent<SpriteRenderer>() != nullptr);
  CHECK(entity.CreateRigidBody() != nullptr);
  CHECK(entity.AddCollider(Collider::CreateSphere(0.5f, 1.0f)));
  CHECK(entity.SetPosition(Vec3(5, 6, 7)));
  CHECK(entity.GetRigidBody()->position.x == 5);
  CHECK(scene.Add(entity));
  CHECK(scene.GetEntityCount() == 1);
  CHECK(scene.FindByName("test entity").GetId() == entity.GetId());
  return 0;
}
