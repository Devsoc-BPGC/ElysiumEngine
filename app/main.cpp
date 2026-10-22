#include "Component.hpp"
#include "GameObject.hpp"
#include "Input.hpp"
#include "SpriteRenderer.hpp"
#include <Elysium.hpp>
#include <cstdlib>
#include <memory>
#include <ScriptingAPI.hpp>

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

class BoxController : public Elysium::Script {
public:
  float speed = 10.0f;

  void OnCreate() override {
    // Tint cyan on start to visually confirm script wrapper control
    entity->SetColor(Elysium::ColorRGBA::Cyan());
  }

  void OnUpdate(float dt) override {
    (void)dt;
    Elysium::Vector2 velocity(0.0f, 0.0f);

    if (Elysium::IsKeyDown("A"))
      velocity.x -= 1.0f;
    if (Elysium::IsKeyDown("D"))
      velocity.x += 1.0f;
    if (Elysium::IsKeyDown("W"))
      velocity.y -= 1.0f;
    if (Elysium::IsKeyDown("S"))
      velocity.y += 1.0f;

    if (velocity.MagnitudeSquared() > 0.0f) {
      velocity = velocity.Normalized() * speed;
      entity->SetVelocity(velocity);
    } else {
      // Apply damping if no directional input
      Elysium::Vector2 currentVel = entity->GetVelocity();
      entity->SetVelocity(currentVel.x * 0.9f, currentVel.y * 0.9f);
    }

    if (Elysium::IsKeyPressed("Space")) {
      entity->ApplyImpulse(0.0f, -8.0f); // Jump impulse
    }
  }
};


class Sandbox : public Elysium::Application {
public:
  Sandbox() : Application("Elysium Game Engine — Editor") {
    ELYSIUM_INFO("Sandbox Application initialized");

    auto *editorLayer = new Elysium::EditorLayer();
    auto &scene = editorLayer->GetScene();
    Elysium::SetActiveScene(&scene);

    // Populate Initial Demo Scene Entities

    // 1. Dynamic Balls
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

    // 2. Dynamic Box with Script Wrapper (WASD Controls)
    {
      auto playerBox = Elysium::SpawnBox("Scripted Box (WASD)", 8.0f, 4.0f, 1.0f, 1.0f);
      playerBox.AttachScript<BoxController>();
    }

    PushOverlay(editorLayer);
  }

  ~Sandbox() override = default;
};

Elysium::Application *Elysium::CreateApplication() { return new Sandbox(); }
