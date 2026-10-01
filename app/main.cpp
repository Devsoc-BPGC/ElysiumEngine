#include "Component.hpp"
#include "GameObject.hpp"
#include "Input.hpp"
#include "SpriteRenderer.hpp"
#include <Elysium.hpp>
#include <cstdlib>
#include <memory>

class LifeSpan : public Component {
public:
  float remaining;
  LifeSpan(float duration) : remaining(duration) {}

  void Update(float dt) override {
    remaining -= dt;
    if (remaining <= 0) {
      gameObject->active = false;
    }
  }
};

class PlayerController : public Component {
public:
  float speed = 10.0f;

  void Update(float dt) override {
    (void)dt;
    if (!gameObject->rigidBody)
      return;

    Vec3 velocity(0, 0, 0);
    if (Input::IsKeyDown(sf::Keyboard::Key::A))
      velocity.x -= 1.0f;
    if (Input::IsKeyDown(sf::Keyboard::Key::D))
      velocity.x += 1.0f;
    if (Input::IsKeyDown(sf::Keyboard::Key::W))
      velocity.y -= 1.0f;
    if (Input::IsKeyDown(sf::Keyboard::Key::S))
      velocity.y += 1.0f;

    if (velocity.MagnitudeSquared() > 0) {
      velocity = velocity.Normalized() * speed;
      gameObject->rigidBody->linearVelocity.x = velocity.x;
      gameObject->rigidBody->linearVelocity.y = velocity.y;
    } else {
      // Apply damping if no input
      gameObject->rigidBody->linearVelocity.x *= 0.9f;
      gameObject->rigidBody->linearVelocity.y *= 0.9f;
    }

    if (Input::IsKeyPressed(sf::Keyboard::Key::Space)) {
      gameObject->rigidBody->linearVelocity.y = -8.0f; // Jump impulse
    }
  }
};

class Sandbox : public Elysium::Application {
public:
  Sandbox() : Application("Elysium Game Engine — Editor") {
    ELYSIUM_INFO("Sandbox Application initialized");

    auto *editorLayer = new Elysium::EditorLayer();
    auto &scene = editorLayer->GetScene();

    // Populate Initial Demo Scene Entities
    // 1. Static Platform
    {
      auto ground = std::make_shared<GameObject>("Ground Platform");
      ground->position = Vec3(8.0f, 10.0f, 0.0f);
      auto &rb = ground->CreateRigidBody();
      rb.isStatic = true;
      rb.mass = 0.0f;
      rb.inverseMass = 0.0f;
      rb.AddColliders(Collider::CreateBox(Vec3(6.0f, 0.4f, 0.0f), 0.0f));
      ground->AddComponent<SpriteRenderer>(sf::Color(80, 140, 90),
                                           sf::Vector2f(12.0f, 0.8f));
      scene.AddGameObject(ground);
    }

    // 2. Dynamic Balls
    for (int i = 0; i < 3; ++i) {
      auto ball =
          std::make_shared<GameObject>("Dynamic Ball " + std::to_string(i));
      ball->position = Vec3(6.0f + static_cast<float>(i) * 2.0f,
                            2.0f + static_cast<float>(i) * 0.5f, 0.0f);
      auto &rb = ball->CreateRigidBody();
      rb.linearVelocity = Vec3(static_cast<float>(i) * 0.4f, 1.0f, 0.0f);
      rb.restitution = 0.75f;
      rb.friction = 0.2f;
      rb.AddColliders(Collider::CreateSphere(0.5f, 1.0f));
      sf::Color ballColor = (i == 0) ? sf::Color(240, 80, 80)
                                     : ((i == 1) ? sf::Color(80, 160, 240)
                                                 : sf::Color(240, 200, 60));
      ball->AddComponent<SpriteRenderer>(ballColor, 0.5f);
      scene.AddGameObject(ball);
    }

    // 3. Dynamic Box with PlayerController
    {
      auto playerBox = std::make_shared<GameObject>("Player Box");
      playerBox->position = Vec3(8.0f, 4.0f, 0.0f);
      auto &rb = playerBox->CreateRigidBody();
      rb.restitution = 0.4f;
      rb.friction = 0.3f;
      rb.AddColliders(Collider::CreateBox(Vec3(0.5f, 0.5f, 0.0f), 1.0f));
      playerBox->AddComponent<SpriteRenderer>(sf::Color(160, 100, 220),
                                              sf::Vector2f(1.0f, 1.0f));
      playerBox->AddComponent<PlayerController>();
      scene.AddGameObject(playerBox);
    }

    PushOverlay(editorLayer);
  }

  ~Sandbox() override = default;
};

Elysium::Application *Elysium::CreateApplication() { return new Sandbox(); }
