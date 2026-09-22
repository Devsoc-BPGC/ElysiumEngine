#ifndef SIMPLERENDERER_HPP
#define SIMPLERENDERER_HPP

#include "BroadPhase.hpp"
#include "Scene.hpp"
#include "SpriteRenderer.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <optional>

class SimpleRenderer {
public:
  float pixelsPerMeter;
  bool drawDebugGrid = false;
  bool drawPhysicsColliders = true;
  bool drawAABBs = false;
  uint32_t drawCalls = 0;
  uint32_t primitivesCount = 0;

  explicit SimpleRenderer(float ppm = 50.0f)
      : pixelsPerMeter(ppm), m_window(nullptr) {}

  SimpleRenderer(sf::RenderWindow &window, float ppm = 50.0f)
      : pixelsPerMeter(ppm), m_window(&window) {}

  void Render(Scene &scene) {
    if (m_window) {
      Render(*m_window, scene);
      m_window->display();
    }
  }

  void Render(sf::RenderTarget &target, Scene &scene,
              const std::optional<sf::View> &view = std::nullopt) {
    drawCalls = 0;
    primitivesCount = 0;

    if (view.has_value()) {
      target.setView(*view);
    } else {
      target.setView(target.getDefaultView());
    }

    target.clear(sf::Color(119, 221, 119));

    float ptm = pixelsPerMeter;

    // Draw Debug Grid
    if (drawDebugGrid) {
      float cellSize = BroadPhase::CELL_SIZE * ptm;
      sf::Vector2u targetSize = target.getSize();

      sf::VertexArray lines(sf::PrimitiveType::Lines);
      sf::Color gridColor(60, 60, 60, 150);

      // Vertical lines
      for (float x = 0; x <= static_cast<float>(targetSize.x); x += cellSize) {
        lines.append(sf::Vertex({x, 0}, gridColor));
        lines.append(sf::Vertex({x, static_cast<float>(targetSize.y)}, gridColor));
      }
      // Horizontal lines
      for (float y = 0; y <= static_cast<float>(targetSize.y); y += cellSize) {
        lines.append(sf::Vertex({0, y}, gridColor));
        lines.append(sf::Vertex({static_cast<float>(targetSize.x), y}, gridColor));
      }
      target.draw(lines);
      drawCalls++;
    }

    // Draw the Boundary Box
    sf::RectangleShape boundaryVisual;
    boundaryVisual.setSize(
        {scene.physicsWorld.boundaryHalfExtents.x * 2.0f * ptm,
         scene.physicsWorld.boundaryHalfExtents.y * 2.0f * ptm});
    boundaryVisual.setOrigin({scene.physicsWorld.boundaryHalfExtents.x * ptm,
                              scene.physicsWorld.boundaryHalfExtents.y * ptm});
    boundaryVisual.setPosition({scene.physicsWorld.boundaryCenter.x * ptm,
                                scene.physicsWorld.boundaryCenter.y * ptm});
    boundaryVisual.setFillColor(sf::Color::Transparent);
    boundaryVisual.setOutlineThickness(5.0f);
    boundaryVisual.setOutlineColor(sf::Color::White);
    boundaryVisual.setRotation(
        sf::radians(scene.physicsWorld.boundaryRotation));
    target.draw(boundaryVisual);
    drawCalls++;
    primitivesCount++;

    for (auto &obj : scene.objects) {
      if (!obj || !obj->active)
        continue;

      // Render SpriteRenderer component if attached
      auto *sprite = obj->GetComponent<SpriteRenderer>();
      if (sprite) {
        if (sprite->isCircle) {
          float screenRadius = sprite->radius * ptm;
          sf::CircleShape shape(screenRadius);
          shape.setFillColor(sprite->color);
          shape.setOrigin({screenRadius, screenRadius});
          shape.setPosition({obj->position.x * ptm, obj->position.y * ptm});
          target.draw(shape);
          drawCalls++;
          primitivesCount++;
        } else {
          sf::RectangleShape shape;
          shape.setSize({sprite->size.x * ptm, sprite->size.y * ptm});
          shape.setOrigin({sprite->size.x * 0.5f * ptm, sprite->size.y * 0.5f * ptm});
          shape.setPosition({obj->position.x * ptm, obj->position.y * ptm});
          shape.setFillColor(sprite->color);
          Quat q = obj->rotation;
          float angle = 2.0f * std::atan2(q.z, q.w) * 180.0f / PI;
          shape.setRotation(sf::degrees(angle));
          target.draw(shape);
          drawCalls++;
          primitivesCount++;
        }
      }

      if (!obj->rigidBody || !drawPhysicsColliders)
        continue;

      auto &body = obj->rigidBody;

      // Draw Colliders
      for (auto &col : body->colliders) {
        Vec3 globalPos = body->LocalToGlobal(col.localCentroid);

        if (col.type == ColliderType::Sphere) {
          float screenRadius = col.radius * ptm;
          sf::CircleShape shape(screenRadius);
          if (sprite) {
            shape.setFillColor(sf::Color::Transparent);
            shape.setOutlineColor(sf::Color::White);
            shape.setOutlineThickness(1.5f);
          } else {
            shape.setFillColor(sf::Color::White);
          }
          shape.setOrigin({screenRadius, screenRadius});
          shape.setPosition({globalPos.x * ptm, globalPos.y * ptm});
          target.draw(shape);
          drawCalls++;
          primitivesCount++;
        } else if (col.type == ColliderType::Box) {
          sf::RectangleShape shape;
          shape.setSize(
              {col.halfExtents.x * 2.0f * ptm, col.halfExtents.y * 2.0f * ptm});
          shape.setOrigin({col.halfExtents.x * ptm, col.halfExtents.y * ptm});
          shape.setPosition({globalPos.x * ptm, globalPos.y * ptm});
          if (sprite) {
            shape.setFillColor(sf::Color::Transparent);
            shape.setOutlineColor(sf::Color(200, 200, 200));
            shape.setOutlineThickness(1.5f);
          } else {
            shape.setFillColor(sf::Color(100, 100, 100)); // Grey for boxes
          }

          // Simple rotation for the box
          Quat q = body->orientation.ToQuat();
          // We only care about Z rotation for 2D SFML
          float angle = 2.0f * std::atan2(q.z, q.w) * 180.0f / PI;
          shape.setRotation(sf::degrees(angle));

          target.draw(shape);
          drawCalls++;
          primitivesCount++;
        }

        if (drawDebugGrid) {
          // Draw Centroid
          sf::CircleShape centroidDot(2.0f);
          centroidDot.setFillColor(sf::Color::Red);
          centroidDot.setOrigin({2.0f, 2.0f});
          centroidDot.setPosition(
              {body->globalCentroid.x * ptm, body->globalCentroid.y * ptm});
          target.draw(centroidDot);
          drawCalls++;
        }

        if (drawAABBs) {
          // Draw AABB (Optional/Debug)
          AABB bounds = body->GetAABB();
          sf::RectangleShape aabbVisual;
          aabbVisual.setPosition({bounds.min.x * ptm, bounds.min.y * ptm});
          aabbVisual.setSize({(bounds.max.x - bounds.min.x) * ptm,
                              (bounds.max.y - bounds.min.y) * ptm});
          aabbVisual.setFillColor(sf::Color::Transparent);
          aabbVisual.setOutlineColor(sf::Color(0, 255, 0, 100));
          aabbVisual.setOutlineThickness(1.0f);
          target.draw(aabbVisual);
          drawCalls++;
        }
      }
    }
  }

private:
  sf::RenderWindow *m_window = nullptr;
};

#endif
