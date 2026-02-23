#include <SFML/Graphics.hpp>
#include "../include/PhysicsWorld.hpp"
#include "../include/RigidBody.hpp"

int main() {
    PhysicsWorld world(Vec3(0.0f, 9.81f, 0.0f));

    RigidBody ball;
    ball.position = Vec3(100.0f, 50.0f, 0.0f);
    ball.linearVelocity = Vec3(5.0f, 0.0f, 0.0f);
    ball.orientation = Mat3::Identity();

    Collider sphere = Collider::CreateSphere(0.5f, 1.0f);
    ball.AddColliders(sphere);

    world.AddBody(&ball);

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Physics Engine Render");
    float pixelsPerMeter = 50.0f;
    sf::Clock clock;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        world.Step(dt);

        window.clear(sf::Color(30, 30, 30));

        for (auto* body : world.rigidBodies) {
            for (auto& col : body->colliders) {
                Vec3 globalPos = body->LocalToGlobal(col.localCentroid);


                float screenRadius = col.radius * pixelsPerMeter;
                sf::CircleShape shape(screenRadius);
                shape.setFillColor(sf::Color::Cyan);
                shape.setOrigin({screenRadius, screenRadius});


                shape.setPosition({globalPos.x * pixelsPerMeter, globalPos.y * pixelsPerMeter});

                window.draw(shape);
            }
        }

        window.display();
    }

    return 0;
}
