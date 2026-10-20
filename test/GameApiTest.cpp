#include "Game.hpp"

#include <cassert>

int main() {
  Elysium::Game::Entity invalid;
  Vec3 position;
  assert(!invalid.IsValid());
  assert(!invalid.GetPosition(position));
  assert(invalid.GetComponent<SpriteRenderer>() == nullptr);
  assert(invalid.CreateRigidBody() == nullptr);
  assert(!invalid.ApplyForce(Vec3(1, 0, 0), Vec3()));

  ::Scene engineScene;
  Elysium::Game::Scene scene(engineScene);
  auto entity = scene.CreateEntity("test entity");
  assert(entity.IsValid());
  assert(entity.SetPosition(Vec3(2, 3, 4)));
  assert(entity.GetPosition(position));
  assert(position.x == 2 && position.y == 3 && position.z == 4);
  assert(!entity.HasComponent<SpriteRenderer>());
  assert(entity.AddComponent<SpriteRenderer>() != nullptr);
  assert(entity.HasComponent<SpriteRenderer>());
  assert(entity.GetComponent<SpriteRenderer>() != nullptr);
  assert(entity.CreateRigidBody() != nullptr);
  assert(entity.AddCollider(Collider::CreateSphere(0.5f, 1.0f)));
  assert(entity.SetPosition(Vec3(5, 6, 7)));
  assert(entity.GetRigidBody()->position.x == 5);
  assert(scene.Add(entity));
  assert(scene.GetEntityCount() == 1);
  assert(scene.FindByName("test entity").GetId() == entity.GetId());
  return 0;
}
