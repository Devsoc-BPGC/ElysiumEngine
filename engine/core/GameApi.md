# Game-facing C++ API

`engine/core/include/Game.hpp` is the public convenience layer for gameplay
code. Include `Game.hpp` and use `Elysium::Game::Scene` and
`Elysium::Game::Entity` to create and manipulate objects without depending on
the editor or renderer internals.

```cpp
::Scene engineScene;
Elysium::Game::Scene scene(engineScene);
auto player = scene.CreateEntity("Player");
player.SetPosition(Vec3(0.0f, 2.0f, 0.0f));
auto *body = player.CreateRigidBody();
body->AddColliders(Collider::CreateBox(Vec3(0.5f, 0.5f, 0.0f), 1.0f));
player.AddComponent<SpriteRenderer>(sf::Color::Green, sf::Vector2f(1, 1));
scene.Add(player); // Add physics bodies before adding the entity to the scene.

if (auto found = scene.FindByName("Player")) {
  Vec3 position;
  found.GetPosition(position);
  found.ApplyForce(Vec3(0, 10, 0), position);
}
```

## Behavior and conventions

- An empty `Entity` is invalid. Getters that write to an output parameter and
  mutating operations return `false` for invalid entities; pointer-returning
  operations return `nullptr`. `GetId()` returns `0`, `GetName()` an empty
  string, `IsActive()` false, and `HasComponent<T>()` false.
- Missing components are represented by `nullptr` from `GetComponent<T>()`.
  `AddComponent<T>()` returns its pointer, or `nullptr` for an invalid entity.
- Physics helpers return `false` when there is no rigid body. Create rigid bodies
  and attach colliders before adding an entity: `::Scene::AddGameObject` registers
  the body's address with the physics world at insertion time.
- Position and rotation setters synchronize an existing rigid body using the
  engine's existing `GameObject::SyncTransform`; scale is stored on the object
  because physics currently has no scale support.
- Collider construction, sprite colors/sizes, and custom component types use
  existing engine types and implementations. No scripting runtime is provided.

## Public and internal surfaces

Use `Game.hpp` for game-facing entity, transform, component, physics, and scene
operations. `GameObject`, `::Scene`, `RigidBody`, `PhysicsWorld`, collider
internals, renderer implementation, and editor APIs remain engine-level types;
the wrapper delegates to them and does not replace them. Lower-level headers
remain available to engine and editor code.

`test/GameApiTest.cpp` is a focused example of the wrapper contract. It is built
with the repository's existing `BUILD_TESTING` CMake option.
