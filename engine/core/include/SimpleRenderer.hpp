#ifndef SIMPLERENDERER_HPP
#define SIMPLERENDERER_HPP

#include <SFML/Graphics.hpp>
#include "Scene.hpp"
#include "BroadPhase.hpp"

class SimpleRenderer {
public:
    sf::RenderWindow& window;
    float pixelsPerMeter;
    bool drawDebugGrid = false;

    SimpleRenderer(sf::RenderWindow& window, float ppm = 50.0f)
        : window(window), pixelsPerMeter(ppm) {}

    void Render(Scene& scene) {
        window.clear(sf::Color(119, 221, 119));

        float ptm = pixelsPerMeter;

        // Draw Debug Grid
        if (drawDebugGrid) {
            float cellSize = BroadPhase::CELL_SIZE * ptm;
            sf::Vector2u windowSize = window.getSize();

            sf::VertexArray lines(sf::PrimitiveType::Lines);
            sf::Color gridColor(60, 60, 60, 150);

            // Vertical lines
            for (float x = 0; x <= windowSize.x; x += cellSize) {
                lines.append(sf::Vertex({x, 0}, gridColor));
                lines.append(sf::Vertex({x, (float)windowSize.y}, gridColor));
            }
            // Horizontal lines
            for (float y = 0; y <= windowSize.y; y += cellSize) {
                lines.append(sf::Vertex({0, y}, gridColor));
                lines.append(sf::Vertex({(float)windowSize.x, y}, gridColor));
            }
            window.draw(lines);
        }

        // Draw the Boundary Box
        sf::RectangleShape boundaryVisual;
        boundaryVisual.setSize({scene.physicsWorld.boundaryHalfExtents.x * 2.0f * ptm, scene.physicsWorld.boundaryHalfExtents.y * 2.0f * ptm});
        boundaryVisual.setOrigin({scene.physicsWorld.boundaryHalfExtents.x * ptm, scene.physicsWorld.boundaryHalfExtents.y * ptm});
        boundaryVisual.setPosition({scene.physicsWorld.boundaryCenter.x * ptm, scene.physicsWorld.boundaryCenter.y * ptm});
        boundaryVisual.setFillColor(sf::Color::Transparent);
        boundaryVisual.setOutlineThickness(5.0f);
        boundaryVisual.setOutlineColor(sf::Color::White);
        boundaryVisual.setRotation(sf::radians(scene.physicsWorld.boundaryRotation));
        window.draw(boundaryVisual);

        for (auto& obj : scene.objects) {
            if (!obj->rigidBody) continue;

            auto& body = obj->rigidBody;

            // Draw Colliders
            for (auto& col : body->colliders) {
                Vec3 globalPos = body->LocalToGlobal(col.localCentroid);
                float ptm = pixelsPerMeter;

                if (col.type == ColliderType::Sphere) {
                    float screenRadius = col.radius * ptm;
                    sf::CircleShape shape(screenRadius);
                    shape.setFillColor(sf::Color::White);
                    shape.setOrigin({screenRadius, screenRadius});
                    shape.setPosition({globalPos.x * ptm, globalPos.y * ptm});
                    window.draw(shape);
                }
                else if (col.type == ColliderType::Box) {
                    sf::RectangleShape shape;
                    shape.setSize({col.halfExtents.x * 2.0f * ptm, col.halfExtents.y * 2.0f * ptm});
                    shape.setOrigin({col.halfExtents.x * ptm, col.halfExtents.y * ptm});
                    shape.setPosition({globalPos.x * ptm, globalPos.y * ptm});
                    shape.setFillColor(sf::Color(100, 100, 100)); // Grey for boxes

                    // Simple rotation for the box
                    Quat q = body->orientation.ToQuat();
                    // We only care about Z rotation for 2D SFML
                    float angle = 2.0f * std::atan2(q.z, q.w) * 180.0f / PI;
                    shape.setRotation(sf::degrees(angle));

                    window.draw(shape);
                }

                if (drawDebugGrid) {
                    // Draw Centroid
                    sf::CircleShape centroidDot(2.0f);
                    centroidDot.setFillColor(sf::Color::Red);
                    centroidDot.setOrigin({2.0f, 2.0f});
                    centroidDot.setPosition({body->globalCentroid.x * ptm, body->globalCentroid.y * ptm});
                    window.draw(centroidDot);

                    // Draw AABB (Optional/Debug)
                    AABB bounds = body->GetAABB();
                    sf::RectangleShape aabbVisual;
                    aabbVisual.setPosition({bounds.min.x * ptm, bounds.min.y * ptm});
                    aabbVisual.setSize({(bounds.max.x - bounds.min.x) * ptm, (bounds.max.y - bounds.min.y) * ptm});
                    aabbVisual.setFillColor(sf::Color::Transparent);
                    aabbVisual.setOutlineColor(sf::Color(0, 255, 0, 100));
                    aabbVisual.setOutlineThickness(1.0f);
                    window.draw(aabbVisual);
                }
            }
        }

        window.display();
    }
};

#endif
