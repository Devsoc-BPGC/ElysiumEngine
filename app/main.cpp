#include "Component.hpp"
#include "EditorLayer.hpp"
#include "GameObject.hpp"
#include "Input.hpp"
#include "Scene.hpp"
#include "SimpleRenderer.hpp"
#include "SpriteRenderer.hpp"
#include <Elysium.hpp>
#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <iostream>
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
  Sandbox() { ELYSIUM_INFO("Sandbox Application started"); }

  ~Sandbox() override = default;

  void Run() override {
    // 1. Setup SFML Window
    sf::RenderWindow window(sf::VideoMode({1280, 720}),
                            "Elysium Game Engine — Editor");
    window.setVerticalSyncEnabled(true);

    // 2. Setup Editor Layer
    Elysium::EditorLayer editorLayer;
    editorLayer.Init(window);

    // 3. Setup Scene & Physics
    Scene scene;
    scene.physicsWorld.gravity = Vec3(0.0f, 9.81f, 0.0f);

    // 4. Populate Initial Scene Entities
    // Static Platform
    {
      auto ground = std::make_shared<GameObject>("Ground Platform");
      ground->position = Vec3(8.0f, 10.0f, 0.0f);
      auto &rb = ground->CreateRigidBody();
      rb.isStatic = true;
      rb.mass = 0.0f;
      rb.inverseMass = 0.0f;
      rb.AddColliders(Collider::CreateBox(Vec3(6.0f, 0.4f, 0.0f), 0.0f));
      ground->AddComponent<SpriteRenderer>(sf::Color(80, 140, 90), sf::Vector2f(12.0f, 0.8f));
      scene.AddGameObject(ground);
    }

    // Dynamic Balls
    for (int i = 0; i < 3; ++i) {
      auto ball = std::make_shared<GameObject>("Dynamic Ball " + std::to_string(i));
      ball->position = Vec3(6.0f + static_cast<float>(i) * 2.0f, 2.0f + static_cast<float>(i) * 0.5f, 0.0f);
      auto &rb = ball->CreateRigidBody();
      rb.linearVelocity = Vec3(static_cast<float>(i) * 0.4f, 1.0f, 0.0f);
      rb.restitution = 0.75f;
      rb.friction = 0.2f;
      rb.AddColliders(Collider::CreateSphere(0.5f, 1.0f));
      sf::Color ballColor = (i == 0) ? sf::Color(240, 80, 80) : ((i == 1) ? sf::Color(80, 160, 240) : sf::Color(240, 200, 60));
      ball->AddComponent<SpriteRenderer>(ballColor, 0.5f);
      scene.AddGameObject(ball);
    }

    // Dynamic Box with PlayerController
    {
      auto playerBox = std::make_shared<GameObject>("Player Box");
      playerBox->position = Vec3(8.0f, 4.0f, 0.0f);
      auto &rb = playerBox->CreateRigidBody();
      rb.restitution = 0.4f;
      rb.friction = 0.3f;
      rb.AddColliders(Collider::CreateBox(Vec3(0.5f, 0.5f, 0.0f), 1.0f));
      playerBox->AddComponent<SpriteRenderer>(sf::Color(160, 100, 220), sf::Vector2f(1.0f, 1.0f));
      playerBox->AddComponent<PlayerController>();
      scene.AddGameObject(playerBox);
    }

    // 5. Setup Renderer
    SimpleRenderer renderer(50.0f);
    sf::Clock clock;

    // Main Engine / Editor Loop
    while (window.isOpen()) {
      Input::Update();

      while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
          window.close();
        }

        editorLayer.ProcessEvent(*event);
        Input::HandleEvent(*event);
      }

      float dt = clock.restart().asSeconds();
      if (dt > 0.1f)
        dt = 0.1f;

      // Update simulation based on editor play state
      sf::Clock updateTimer;
      if (editorLayer.GetContext().playState == Elysium::ScenePlayState::Play) {
        scene.physicsWorld.boundaryRotation += 0.0005f;
        scene.Update(dt);
      } else if (editorLayer.GetContext().playState == Elysium::ScenePlayState::Step) {
        scene.Update(1.0f / 60.0f);
        editorLayer.GetContext().playState = Elysium::ScenePlayState::Pause;
      }
      float updateMs = updateTimer.getElapsedTime().asSeconds() * 1000.0f;
      editorLayer.GetContext().stats.updateTimeMs = updateMs;
      editorLayer.GetContext().stats.physicsTimeMs = updateMs * 0.75f;

      // Render Editor UI and Viewport
      sf::Clock renderTimer;
      window.clear(sf::Color(22, 22, 26));

      editorLayer.BeginFrame(dt);
      editorLayer.RenderPanels(scene, renderer);
      editorLayer.EndFrame();

      window.display();

      float renderMs = renderTimer.getElapsedTime().asSeconds() * 1000.0f;
      editorLayer.GetContext().stats.renderTimeMs = renderMs;
      editorLayer.GetContext().stats.frameTimeMs = dt * 1000.0f;
      editorLayer.GetContext().stats.fps = (dt > 0.0001f) ? (1.0f / dt) : 60.0f;
      editorLayer.GetContext().stats.drawCalls = renderer.drawCalls;
    }

    editorLayer.Shutdown();
  }
};

Elysium::Application *Elysium::CreateApplication() {
  return new Sandbox();
}
