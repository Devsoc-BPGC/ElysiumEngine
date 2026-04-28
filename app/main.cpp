#include <SFML/Graphics.hpp>
#include <memory>
#include <cstdlib>
#include "Scene.hpp"
#include "GameObject.hpp"
#include "SimpleRenderer.hpp"
#include "Component.hpp"

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

int main() {
    // 1. Setup Scene
    Scene scene;
    scene.physicsWorld.gravity = Vec3(0.0f, 9.81f, 0.0f);

    // 2. Create Initial Dynamic Balls
    for (int i = 0; i < 5; ++i) {
        auto ball = std::make_shared<GameObject>("Ball " + std::to_string(i));
        ball->position = Vec3(3.0f + i * 2.0f, 3.0f, 0.0f);
        auto& rb = ball->CreateRigidBody();
        rb.linearVelocity = Vec3(0.5f * i, 5.0f, 0.0f);
        rb.AddColliders(Collider::CreateSphere(0.5f, 1.0f));
        scene.AddGameObject(ball);
    }

    // 3. Setup Window and Renderer
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Elysium Engine - Reverted Boundaries");
    SimpleRenderer renderer(window, 50.0f);
    sf::Clock clock;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            
            // Spawn in the middle (8m, 6m) on Space hit
            if (event->is<sf::Event::KeyPressed>()) {
                if (event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Space) {
                    auto ball = std::make_shared<GameObject>("New Ball");
                    // Middle of the 800x600 window (at 50ppm) is (8, 6)
                    ball->position = Vec3(8.0f, 6.0f, 0.0f);
                    auto& rb = ball->CreateRigidBody();
                    rb.linearVelocity = Vec3((rand() % 20 - 10), (rand() % 20 - 10), 0);
                    rb.AddColliders(Collider::CreateSphere(0.4f, 1.0f));
                    
                    ball->AddComponent<LifeSpan>(3.0f);
                    scene.AddGameObject(ball);
                }
            }
        }

        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        scene.Update(dt);
        renderer.Render(scene);
    }

    return 0;
}
