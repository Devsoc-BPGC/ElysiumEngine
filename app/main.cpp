#include <SFML/Graphics.hpp>
#include <memory>
#include <cstdlib>
#include <iostream>
#include "Scene.hpp"
#include "GameObject.hpp"
#include "SimpleRenderer.hpp"
#include "Component.hpp"
#include "Input.hpp"

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
        if (!gameObject->rigidBody) return;

        Vec3 velocity(0, 0, 0);
        if (Input::IsKeyDown(sf::Keyboard::Key::A)) velocity.x -= 1.0f;
        if (Input::IsKeyDown(sf::Keyboard::Key::D)) velocity.x += 1.0f;
        if (Input::IsKeyDown(sf::Keyboard::Key::W)) velocity.y -= 1.0f;
        if (Input::IsKeyDown(sf::Keyboard::Key::S)) velocity.y += 1.0f;

        if (velocity.MagnitudeSquared() > 0) {
            velocity = velocity.Normalized() * speed;
            gameObject->rigidBody->linearVelocity.x = velocity.x;
            gameObject->rigidBody->linearVelocity.y = velocity.y;
        } else {
            // Apply some damping if no input
            gameObject->rigidBody->linearVelocity.x *= 0.9f;
            gameObject->rigidBody->linearVelocity.y *= 0.9f;
        }

        if (Input::IsKeyPressed(sf::Keyboard::Key::Space)) {
            gameObject->rigidBody->linearVelocity.y = -10.0f; // Jump!
        }
    }
};

int main() {
    // 1. Setup Scene
    Scene scene;
    scene.physicsWorld.gravity = Vec3(0.0f, 9.81f, 0.0f);

    // 2. Create Player
    /*
    auto player = std::make_shared<GameObject>("Player");
    player->position = Vec3(4.0f, 4.0f, 0.0f);
    auto& playerRB = player->CreateRigidBody();
    playerRB.AddColliders(Collider::CreateBox(Vec3(0.5f, 0.5f, 0.5f), 1.0f));
    player->AddComponent<PlayerController>();
    scene.AddGameObject(player);
    */

    // 3. Create Initial Dynamic Balls
    for (int i = 0; i < 3; ++i) {
        auto ball = std::make_shared<GameObject>("Ball " + std::to_string(i));
        ball->position = Vec3(3.0f + i * 2.0f, 2.0f, 0.0f);
        auto& rb = ball->CreateRigidBody();
        rb.linearVelocity = Vec3(0.5f * i, 2.0f, 0.0f);
        rb.AddColliders(Collider::CreateSphere(0.5f, 1.0f));
        scene.AddGameObject(ball);
    }

    // 4. Setup Window and Renderer
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Elysium Engine - Input System");
    SimpleRenderer renderer(window, 50.0f);
    sf::Clock clock;

    while (window.isOpen()) {
        Input::Update();

        while (const std::optional event = window.pollEvent()) {
            Input::HandleEvent(*event);

            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::G) {
                    renderer.drawDebugGrid = !renderer.drawDebugGrid;
                }
            }

            // Handle Mouse Wheel for Boundary Rotation
            float wheelDelta = Input::GetMouseWheelDelta();
            if (std::abs(wheelDelta) > 0.01f) {
                scene.physicsWorld.boundaryRotation += wheelDelta * 0.05f;
            }

            // Spawn in the middle (8m, 6m) on Mouse Click
            if (Input::IsMouseButtonPressed(sf::Mouse::Button::Left)) {
                auto ball = std::make_shared<GameObject>("New Ball");
                sf::Vector2f mousePos = Input::GetMousePosition();
                ball->position = Vec3(mousePos.x / 50.0f, mousePos.y / 50.0f, 0.0f);

                auto& rb = ball->CreateRigidBody();
                rb.linearVelocity = Vec3((rand() % 10 - 5), (rand() % 10 - 5), 0);
                rb.AddColliders(Collider::CreateSphere(0.4f, 1.0f));

                ball->AddComponent<LifeSpan>(5.0f);
                scene.AddGameObject(ball);
            }
        }

        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        scene.Update(dt);
        renderer.Render(scene);
    }

    return 0;
}
