/**
 * @file SimpleRenderer.cpp
 * @brief Implementation of 2D mouse picking and raycasting for Elysium Engine.
 */

#include "SimpleRenderer.hpp"
#include <cmath>
#include <limits>

namespace {

bool IsPointInEntity(const GameObject &obj, const Vec2 &point) {
  // 1. Test SpriteRenderer if attached
  auto *sprite = const_cast<GameObject &>(obj).GetComponent<SpriteRenderer>();
  if (sprite) {
    Vec3 objWorldPos = obj.GetWorldPosition();
    Quat q = obj.GetWorldRotation();
    Vec3 objWorldScale = obj.GetWorldScale();

    float objAngleDeg = 2.0f * std::atan2(q.z, q.w) * 180.0f / PI;
    float totalAngleDeg = objAngleDeg + sprite->LocalRotationOffset;
    float totalAngleRad = totalAngleDeg * PI / 180.0f;

    Vec3 scaledOffset(sprite->LocalPositionOffset.x * objWorldScale.x,
                      sprite->LocalPositionOffset.y * objWorldScale.y, 0.0f);
    Vec3 worldPos = objWorldPos + q.RotateVector(scaledOffset);

    Vec2 compoundScale(objWorldScale.x * sprite->LocalScaleOffset.x,
                       objWorldScale.y * sprite->LocalScaleOffset.y);

    // Transform world point into sprite's local rotated space
    float dx = point.x - worldPos.x;
    float dy = point.y - worldPos.y;
    float cosA = std::cos(-totalAngleRad);
    float sinA = std::sin(-totalAngleRad);
    float localX = dx * cosA - dy * sinA;
    float localY = dx * sinA + dy * cosA;

    if (sprite->isCircle) {
      float r = sprite->radius * std::abs(compoundScale.x);
      if (r <= 0.0f) r = 0.5f;
      float cx = (0.5f - sprite->Pivot.x) * 2.0f * r;
      float cy = (0.5f - sprite->Pivot.y) * 2.0f * r;
      float distSq = (localX - cx) * (localX - cx) + (localY - cy) * (localY - cy);
      if (distSq <= r * r) {
        return true;
      }
    } else {
      float w = sprite->size.x * std::abs(compoundScale.x);
      float h = sprite->size.y * std::abs(compoundScale.y);
      if (w <= 0.0f) w = 1.0f;
      if (h <= 0.0f) h = 1.0f;

      float minX = -w * sprite->Pivot.x;
      float maxX = minX + w;
      float minY = -h * sprite->Pivot.y;
      float maxY = minY + h;

      if (localX >= minX && localX <= maxX && localY >= minY && localY <= maxY) {
        return true;
      }
    }
  }

  // 2. Test Physics Colliders if RigidBody is attached
  if (obj.rigidBody && !obj.rigidBody->colliders.empty()) {
    const auto &body = obj.rigidBody;
    for (const auto &col : body->colliders) {
      Vec3 globalPos = body->LocalToGlobal(col.localCentroid);

      if (col.type == ColliderType::Sphere) {
        float distSq = (point.x - globalPos.x) * (point.x - globalPos.x) +
                       (point.y - globalPos.y) * (point.y - globalPos.y);
        if (distSq <= col.radius * col.radius) {
          return true;
        }
      } else if (col.type == ColliderType::Box) {
        Quat q = body->orientation.ToQuat();
        float angleRad = 2.0f * std::atan2(q.z, q.w);

        float dx = point.x - globalPos.x;
        float dy = point.y - globalPos.y;
        float cosA = std::cos(-angleRad);
        float sinA = std::sin(-angleRad);
        float lx = std::abs(dx * cosA - dy * sinA);
        float ly = std::abs(dx * sinA + dy * cosA);

        if (lx <= col.halfExtents.x && ly <= col.halfExtents.y) {
          return true;
        }
      } else if (col.type == ColliderType::Polygon) {
        auto verts = col.GetWorldVertices(*body);
        if (verts.size() >= 3) {
          bool inside = false;
          size_t j = verts.size() - 1;
          for (size_t i = 0; i < verts.size(); ++i) {
            if ((verts[i].y > point.y) != (verts[j].y > point.y) &&
                (point.x < (verts[j].x - verts[i].x) * (point.y - verts[i].y) /
                               (verts[j].y - verts[i].y) +
                           verts[i].x)) {
              inside = !inside;
            }
            j = i;
          }
          if (inside) {
            return true;
          }
        }
      }
    }
  }

  // 3. Test CameraComponent bounds (1.0m x 1.0m around center)
  auto *cam = const_cast<GameObject &>(obj).GetComponent<Elysium::CameraComponent>();
  if (cam) {
    Vec3 worldPos = obj.GetWorldPosition();
    float halfSize = 0.5f;
    if (std::abs(point.x - worldPos.x) <= halfSize &&
        std::abs(point.y - worldPos.y) <= halfSize) {
      return true;
    }
  }

  // 4. Fallback hit bounds for empty / transform-only entities (0.6m x 0.6m)
  if (!sprite && (!obj.rigidBody || obj.rigidBody->colliders.empty())) {
    Vec3 worldPos = obj.GetWorldPosition();
    float halfSize = 0.3f;
    if (std::abs(point.x - worldPos.x) <= halfSize &&
        std::abs(point.y - worldPos.y) <= halfSize) {
      return true;
    }
  }

  return false;
}

} // anonymous namespace

std::shared_ptr<GameObject> SimpleRenderer::PickEntity(Scene &scene,
                                                       const Vec2 &worldPoint) const {
  std::shared_ptr<GameObject> bestHit = nullptr;
  float bestZ = -std::numeric_limits<float>::infinity();
  size_t bestIndex = 0;

  for (size_t i = 0; i < scene.objects.size(); ++i) {
    const auto &obj = scene.objects[i];
    if (!obj || !obj->active) {
      continue;
    }

    if (IsPointInEntity(*obj, worldPoint)) {
      float z = obj->GetWorldPosition().z;
      // Prioritize higher Z-depth; if equal, prioritize later draw order (rendered on top)
      if (!bestHit || z > bestZ || (z == bestZ && i >= bestIndex)) {
        bestHit = obj;
        bestZ = z;
        bestIndex = i;
      }
    }
  }

  return bestHit;
}
