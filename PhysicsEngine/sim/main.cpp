#include <SFML/Graphics.hpp>
#include <optional>
#include "../include/PhysicsWorld.hpp"
#include "../include/RigidBody.hpp"

int main() {
    // 1. Setup Physics World (Gravity in meters/sec^2)
    PhysicsWorld world(Vec3(0.0f, 9.81f, 0.0f));

    // 2. Setup Ball 1 (Heavier, moves right)
    RigidBody ball1;
    ball1.orientation = Mat3::Identity();
    ball1.inverseOrientation = Mat3::Identity();
    ball1.position = Vec3(3.0f, 5.0f, 0.0f); // Inside the box
    ball1.linearVelocity = Vec3(5.0f, 0.0f, 0.0f);

    Collider col1 = Collider::CreateSphere(0.8f, 2.0f);
    ball1.AddColliders(col1);
    ball1.UpdateGlobalCentroidFromPosition();
    world.AddBody(&ball1);

    // 3. Setup Ball 2 (Lighter, moves left)
    RigidBody ball2;
    ball2.orientation = Mat3::Identity();
    ball2.inverseOrientation = Mat3::Identity();
    ball2.position = Vec3(12.0f, 5.0f, 0.0f); // Inside the box
    ball2.linearVelocity = Vec3(-5.0f, 0.0f, 0.0f);

    Collider col2 = Collider::CreateSphere(0.5f, 1.0f);
    ball2.AddColliders(col2);
    ball2.UpdateGlobalCentroidFromPosition();
    world.AddBody(&ball2);

    // 4. SFML Window Setup
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Physics Engine - Ball in Box");
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

        // Draw the Box Boundary (1m to 15m X, 1m to 11m Y)
        float ptm = pixelsPerMeter;
        sf::RectangleShape boxVisual;
        boxVisual.setSize({(15.0f - 1.0f) * ptm, (11.0f - 1.0f) * ptm});
        boxVisual.setPosition({1.0f * ptm, 1.0f * ptm});
        boxVisual.setFillColor(sf::Color::Transparent);
        boxVisual.setOutlineThickness(2.0f);
        boxVisual.setOutlineColor(sf::Color::White);
        window.draw(boxVisual);

        // Draw the Bodies
        for (auto* body : world.rigidBodies) {
            // --- 1. Draw AABB (Green Box) ---
            AABB bounds = body->GetAABB();
            sf::RectangleShape aabbVisual;
            aabbVisual.setPosition({bounds.min.x * ptm, bounds.min.y * ptm});
            aabbVisual.setSize({(bounds.max.x - bounds.min.x) * ptm, (bounds.max.y - bounds.min.y) * ptm});
            aabbVisual.setFillColor(sf::Color::Transparent);
            aabbVisual.setOutlineColor(sf::Color::Green);
            aabbVisual.setOutlineThickness(1.0f);
            window.draw(aabbVisual);

            // --- 2. Draw Colliders (The actual shapes) ---
            for (auto& col : body->colliders) {
                Vec3 globalPos = body->LocalToGlobal(col.localCentroid);
                float screenRadius = col.radius * ptm;
                sf::CircleShape shape(screenRadius);
                shape.setFillColor(sf::Color(0, 255, 255, 100));
                shape.setOrigin({screenRadius, screenRadius});
                shape.setPosition({globalPos.x * ptm, globalPos.y * ptm});
                window.draw(shape);
            }

            for (auto& col : body->colliders) {
                Vec3 globalPos = body->LocalToGlobal(col.localCentroid);

                float screenRadius = col.radius * ptm;
                sf::CircleShape shape(screenRadius);
                shape.setFillColor(sf::Color::Cyan);
                shape.setOrigin({screenRadius, screenRadius});

                shape.setPosition({globalPos.x * ptm, globalPos.y * ptm});

                window.draw(shape);

                // --- 3. Draw Global Centroid (Red Dot) ---
                sf::CircleShape centroidDot(3.0f); // 3 pixel dot
                centroidDot.setFillColor(sf::Color::Red);
                centroidDot.setOrigin({3.0f, 3.0f});
                centroidDot.setPosition({body->globalCentroid.x * ptm, body->globalCentroid.y * ptm});
                window.draw(centroidDot);
            }
        }

        window.display();
    }

    return 0;
}
