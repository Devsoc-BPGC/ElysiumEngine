#ifndef SIMPLERENDERER_HPP
#define SIMPLERENDERER_HPP

#include "BroadPhase.hpp"
#include "CameraComponent.hpp"
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
  bool drawCameraFrustum = true;
  sf::Color clearColor = sf::Color(119, 221, 119);
  uint32_t drawCalls = 0;
  uint32_t primitivesCount = 0;
  std::shared_ptr<GameObject> selectedEntity = nullptr;

  /**
   * @brief Performs a 2D point-in-bounds hit-test across all active entities in the scene.
   * Evaluates SpriteRenderer geometries, physics colliders, and fallback transform bounds.
   * Selects the top-most overlapping entity (highest Z-index / layer order).
   * @param scene The active scene containing entities.
   * @param worldPoint Coordinates in 2D world space (meters).
   * @return Pointer to top-most hit GameObject or nullptr if clicked on empty space.
   */
  std::shared_ptr<GameObject> PickEntity(Scene &scene, const Vec2 &worldPoint) const;

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

    target.clear(clearColor);

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
        lines.append(
            sf::Vertex({x, static_cast<float>(targetSize.y)}, gridColor));
      }
      // Horizontal lines
      for (float y = 0; y <= static_cast<float>(targetSize.y); y += cellSize) {
        lines.append(sf::Vertex({0, y}, gridColor));
        lines.append(
            sf::Vertex({static_cast<float>(targetSize.x), y}, gridColor));
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
        // Calculate compound transform using world transform
        Vec3 objWorldPos = obj->GetWorldPosition();
        Quat q = obj->GetWorldRotation();
        Vec3 objWorldScale = obj->GetWorldScale();

        float objAngleDeg = 2.0f * std::atan2(q.z, q.w) * 180.0f / PI;
        float totalAngleDeg = objAngleDeg + sprite->LocalRotationOffset;

        // Position: world position + rotated & scaled local offset
        Vec3 scaledOffset(sprite->LocalPositionOffset.x * objWorldScale.x,
                          sprite->LocalPositionOffset.y * objWorldScale.y, 0.0f);
        Vec3 worldPos = objWorldPos + q.RotateVector(scaledOffset);
        sf::Vector2f screenPos(worldPos.x * ptm, worldPos.y * ptm);

        // Compound scale
        sf::Vector2f compoundScale(objWorldScale.x * sprite->LocalScaleOffset.x,
                                  objWorldScale.y * sprite->LocalScaleOffset.y);

        if (sprite->isTextured) {
          sf::Sprite spriteRenderer(sprite->texture->GetTexture());
          // Size in pixels based on texture size or custom size
          auto texSize = sprite->texture->GetTexture().getSize();
          float texW = static_cast<float>(texSize.x);
          float texH = static_cast<float>(texSize.y);
          spriteRenderer.setOrigin({texW * sprite->Pivot.x,
                                   texH * sprite->Pivot.y});
          // Texture scale must also account for sprite->size relative to texture dimensions
          float sx = (sprite->size.x * ptm) / (texW > 0.0f ? texW : 1.0f);
          float sy = (sprite->size.y * ptm) / (texH > 0.0f ? texH : 1.0f);
          spriteRenderer.setScale({compoundScale.x * sx, compoundScale.y * sy});
          spriteRenderer.setPosition(screenPos);
          spriteRenderer.setColor(sprite->color);
          spriteRenderer.setRotation(sf::degrees(totalAngleDeg));
          target.draw(spriteRenderer);
          drawCalls++;
        } else {
          if (sprite->isCircle) {
            float screenRadius = sprite->radius * ptm;
            sf::CircleShape shape(screenRadius);
            shape.setFillColor(sprite->color);
            // Pivot on circle: bounding box width and height is 2 * screenRadius
            shape.setOrigin({2.0f * screenRadius * sprite->Pivot.x,
                             2.0f * screenRadius * sprite->Pivot.y});
            shape.setScale(compoundScale);
            shape.setPosition(screenPos);
            shape.setRotation(sf::degrees(totalAngleDeg));
            target.draw(shape);
            drawCalls++;
            primitivesCount++;
          } else {
            sf::RectangleShape shape;
            float w = sprite->size.x * ptm;
            float h = sprite->size.y * ptm;
            shape.setSize({w, h});
            shape.setOrigin({w * sprite->Pivot.x, h * sprite->Pivot.y});
            shape.setScale(compoundScale);
            shape.setPosition(screenPos);
            shape.setFillColor(sprite->color);
            shape.setRotation(sf::degrees(totalAngleDeg));
            target.draw(shape);
            drawCalls++;
            primitivesCount++;
          }
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

    // Draw Camera Frustum Gizmos in Edit mode / debug mode
    if (drawCameraFrustum) {
      for (const auto &obj : scene.objects) {
        if (!obj || !obj->active)
          continue;
        const auto *cam = const_cast<GameObject *>(obj.get())->GetComponent<Elysium::CameraComponent>();
        if (cam) {
          DrawCameraFrustum(target, *obj, *cam, ptm);
        }
      }
    }

    // Draw selection highlight for the active selected entity
    if (selectedEntity && selectedEntity->active) {
      DrawSelectionHighlight(target, *selectedEntity, ptm);
    }
  }

  void DrawCameraFrustum(sf::RenderTarget &target, const GameObject &cameraEntity,
                         const Elysium::CameraComponent &cameraComp, float ptm) {
    Vec3 camPos = cameraEntity.GetWorldPosition();
    Quat q = cameraEntity.GetWorldRotation();
    float angleDeg = 2.0f * std::atan2(q.z, q.w) * 180.0f / PI;

    float h = cameraComp.Camera.GetOrthographicSize() * ptm;
    float w = cameraComp.Camera.GetOrthographicSize() * cameraComp.Camera.GetAspectRatio() * ptm;

    sf::RectangleShape frustumVisual({w, h});
    frustumVisual.setOrigin({w * 0.5f, h * 0.5f});
    frustumVisual.setPosition({camPos.x * ptm, camPos.y * ptm});
    frustumVisual.setRotation(sf::degrees(angleDeg));
    frustumVisual.setFillColor(sf::Color::Transparent);

    bool isSelected = (selectedEntity.get() == &cameraEntity);
    if (isSelected) {
      frustumVisual.setOutlineColor(sf::Color(50, 200, 255, 230)); // Bright cyan
      frustumVisual.setOutlineThickness(2.5f);
    } else if (cameraComp.Primary) {
      frustumVisual.setOutlineColor(sf::Color(50, 180, 240, 160)); // Cyan for primary
      frustumVisual.setOutlineThickness(1.5f);
    } else {
      frustumVisual.setOutlineColor(sf::Color(150, 150, 150, 120)); // Dim grey for non-primary
      frustumVisual.setOutlineThickness(1.0f);
    }
    target.draw(frustumVisual);

    // Top lens/direction notch on frustum
    float notchW = std::min(w * 0.25f, 24.0f);
    sf::RectangleShape notch({notchW, 6.0f});
    notch.setOrigin({notchW * 0.5f, h * 0.5f + 6.0f});
    notch.setPosition({camPos.x * ptm, camPos.y * ptm});
    notch.setRotation(sf::degrees(angleDeg));
    notch.setFillColor(isSelected ? sf::Color(50, 200, 255, 230)
                                  : sf::Color(50, 180, 240, 160));
    target.draw(notch);

    drawCalls += 2;
    primitivesCount += 2;
  }

private:
  void DrawSelectionHighlight(sf::RenderTarget &target, const GameObject &obj, float ptm) {
    auto *sprite = const_cast<GameObject&>(obj).GetComponent<SpriteRenderer>();
    Vec3 objWorldPos = obj.GetWorldPosition();
    Quat q = obj.GetWorldRotation();
    Vec3 objWorldScale = obj.GetWorldScale();
    float objAngleDeg = 2.0f * std::atan2(q.z, q.w) * 180.0f / PI;

    sf::Color selectOutlineColor(255, 215, 0, 240); // Golden yellow

    if (sprite) {
      float totalAngleDeg = objAngleDeg + sprite->LocalRotationOffset;
      Vec3 scaledOffset(sprite->LocalPositionOffset.x * objWorldScale.x,
                        sprite->LocalPositionOffset.y * objWorldScale.y, 0.0f);
      Vec3 worldPos = objWorldPos + q.RotateVector(scaledOffset);
      sf::Vector2f screenPos(worldPos.x * ptm, worldPos.y * ptm);
      sf::Vector2f compoundScale(objWorldScale.x * sprite->LocalScaleOffset.x,
                                objWorldScale.y * sprite->LocalScaleOffset.y);

      if (sprite->isCircle) {
        float r = sprite->radius * std::abs(compoundScale.x) * ptm + 3.0f;
        sf::CircleShape highlight(r);
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(selectOutlineColor);
        highlight.setOutlineThickness(2.0f);
        highlight.setOrigin({r, r});
        highlight.setPosition(screenPos);
        target.draw(highlight);
      } else {
        float w = sprite->size.x * std::abs(compoundScale.x) * ptm + 6.0f;
        float h = sprite->size.y * std::abs(compoundScale.y) * ptm + 6.0f;
        sf::RectangleShape highlight({w, h});
        highlight.setOrigin({w * sprite->Pivot.x, h * sprite->Pivot.y});
        highlight.setPosition(screenPos);
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(selectOutlineColor);
        highlight.setOutlineThickness(2.0f);
        highlight.setRotation(sf::degrees(totalAngleDeg));
        target.draw(highlight);
      }
      drawCalls++;
    } else if (obj.rigidBody && !obj.rigidBody->colliders.empty()) {
      auto &body = obj.rigidBody;
      for (const auto &col : body->colliders) {
        Vec3 globalPos = body->LocalToGlobal(col.localCentroid);
        if (col.type == ColliderType::Sphere) {
          float r = col.radius * ptm + 3.0f;
          sf::CircleShape highlight(r);
          highlight.setFillColor(sf::Color::Transparent);
          highlight.setOutlineColor(selectOutlineColor);
          highlight.setOutlineThickness(2.0f);
          highlight.setOrigin({r, r});
          highlight.setPosition({globalPos.x * ptm, globalPos.y * ptm});
          target.draw(highlight);
        } else if (col.type == ColliderType::Box) {
          float w = col.halfExtents.x * 2.0f * ptm + 6.0f;
          float h = col.halfExtents.y * 2.0f * ptm + 6.0f;
          sf::RectangleShape highlight({w, h});
          highlight.setOrigin({w * 0.5f, h * 0.5f});
          highlight.setPosition({globalPos.x * ptm, globalPos.y * ptm});
          highlight.setFillColor(sf::Color::Transparent);
          highlight.setOutlineColor(selectOutlineColor);
          highlight.setOutlineThickness(2.0f);
          Quat bq = body->orientation.ToQuat();
          float angle = 2.0f * std::atan2(bq.z, bq.w) * 180.0f / PI;
          highlight.setRotation(sf::degrees(angle));
          target.draw(highlight);
        }
        drawCalls++;
      }
    } else {
      // Fallback for transform-only entities
      float size = 0.6f * ptm;
      sf::RectangleShape highlight({size, size});
      highlight.setOrigin({size * 0.5f, size * 0.5f});
      highlight.setPosition({objWorldPos.x * ptm, objWorldPos.y * ptm});
      highlight.setFillColor(sf::Color::Transparent);
      highlight.setOutlineColor(selectOutlineColor);
      highlight.setOutlineThickness(2.0f);
      highlight.setRotation(sf::degrees(objAngleDeg));
      target.draw(highlight);
      drawCalls++;
    }
  }

  sf::RenderWindow *m_window = nullptr;
};

#endif
