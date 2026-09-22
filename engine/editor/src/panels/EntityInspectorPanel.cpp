#include "panels/EntityInspectorPanel.hpp"
#include "Collider.hpp"
#include "Log.h"
#include "SpriteRenderer.hpp"
#include <cmath>
#include <cstring>
#include <imgui.h>

namespace Elysium {

static constexpr float PI_VAL = 3.14159265358979323846f;

EntityInspectorPanel::EntityInspectorPanel() : EditorPanel("Entity Inspector") {}

void EntityInspectorPanel::OnImGuiRender(EditorContext &context, Scene &scene,
                                        SimpleRenderer &renderer) {
  (void)renderer;

  if (ImGui::Begin("Entity Inspector", &isOpen)) {
    if (!context.selectedEntity) {
      ImGui::Spacing();
      ImGui::TextDisabled("No Entity Selected.");
      ImGui::TextWrapped("Select an entity from the Scene Hierarchy to view and edit its components.");
      ImGui::End();
      return;
    }

    auto &entity = *context.selectedEntity;

    // Entity Header Info
    char nameBuffer[256];
    std::strncpy(nameBuffer, entity.name.c_str(), sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';
    if (ImGui::InputText("Entity Name", nameBuffer, sizeof(nameBuffer))) {
      entity.name = nameBuffer;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Active", &entity.active);

    ImGui::TextDisabled("Entity ID: %u", entity.id);
    ImGui::Separator();

    // 1. Transform Component
    DrawTransformComponent(context, entity);

    // 2. RigidBody2D Component
    DrawRigidBodyComponent(context, scene, entity);

    // 3. Collider2D Component
    DrawCollidersComponent(context, entity);

    // 4. SpriteRenderer Component
    DrawSpriteRendererComponent(context, entity);

    // 5. Custom Components
    DrawCustomComponents(context, entity);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Add Component Button
    if (ImGui::Button("+ Add Component", ImVec2(-1.0f, 0.0f))) {
      ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
      if (!entity.rigidBody && ImGui::MenuItem("RigidBody2D")) {
        auto &rb = entity.CreateRigidBody();
        scene.physicsWorld.AddBody(&rb);
        ELYSIUM_INFO("Added RigidBody2D to entity: {}", entity.name);
      }

      if (entity.rigidBody && ImGui::MenuItem("Sphere Collider")) {
        entity.rigidBody->AddColliders(Collider::CreateSphere(0.5f, 1.0f));
        ELYSIUM_INFO("Added Sphere Collider to entity: {}", entity.name);
      }

      if (entity.rigidBody && ImGui::MenuItem("Box Collider")) {
        entity.rigidBody->AddColliders(Collider::CreateBox(Vec3(0.5f, 0.5f, 0.0f), 1.0f));
        ELYSIUM_INFO("Added Box Collider to entity: {}", entity.name);
      }

      if (!entity.GetComponent<SpriteRenderer>() && ImGui::MenuItem("Sprite Renderer")) {
        entity.AddComponent<SpriteRenderer>(sf::Color(255, 100, 100), sf::Vector2f(1.0f, 1.0f));
        ELYSIUM_INFO("Added SpriteRenderer to entity: {}", entity.name);
      }

      ImGui::EndPopup();
    }
  }
  ImGui::End();
}

void EntityInspectorPanel::DrawTransformComponent(EditorContext &context,
                                                 GameObject &entity) {
  (void)context;
  if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
    bool changed = false;

    // Position (meters)
    float pos[3] = {entity.position.x, entity.position.y, entity.position.z};
    if (ImGui::DragFloat3("Position (m)", pos, 0.05f)) {
      entity.position = Vec3(pos[0], pos[1], pos[2]);
      changed = true;
    }

    // Rotation (degrees around Z axis in 2D)
    Quat q = entity.rotation;
    float angleDeg = 2.0f * std::atan2(q.z, q.w) * 180.0f / PI_VAL;
    if (ImGui::DragFloat("Rotation (deg)", &angleDeg, 1.0f, -360.0f, 360.0f)) {
      float rad = angleDeg * PI_VAL / 180.0f;
      entity.rotation = Quat(Vec3(0, 0, 1), rad);
      changed = true;
    }

    // Scale
    float scale[3] = {entity.scale.x, entity.scale.y, entity.scale.z};
    if (ImGui::DragFloat3("Scale", scale, 0.05f, 0.01f, 100.0f)) {
      entity.scale = Vec3(scale[0], scale[1], scale[2]);
      changed = true;
    }

    // Immediately synchronize physics state with the transform changes
    if (changed) {
      entity.SyncTransform();
    }
  }
}

void EntityInspectorPanel::DrawRigidBodyComponent(EditorContext &context,
                                                 Scene &scene,
                                                 GameObject &entity) {
  (void)context;
  if (!entity.rigidBody)
    return;

  auto &body = entity.rigidBody;

  if (ImGui::CollapsingHeader("RigidBody2D", ImGuiTreeNodeFlags_DefaultOpen)) {
    // Body Type
    bool isStatic = body->isStatic;
    if (ImGui::Checkbox("Is Static", &isStatic)) {
      body->isStatic = isStatic;
      if (isStatic) {
        body->mass = 0.0f;
        body->inverseMass = 0.0f;
        body->linearVelocity = Vec3(0, 0, 0);
        body->angularVelocity = Vec3(0, 0, 0);
      } else {
        body->mass = (body->mass <= 0.0f) ? 1.0f : body->mass;
        body->inverseMass = 1.0f / body->mass;
      }
    }

    if (!body->isStatic) {
      // Mass
      if (ImGui::DragFloat("Mass (kg)", &body->mass, 0.1f, 0.01f, 5000.0f)) {
        if (body->mass > 0.0f) {
          body->inverseMass = 1.0f / body->mass;
        }
      }

      // Linear Velocity
      float vel[2] = {body->linearVelocity.x, body->linearVelocity.y};
      if (ImGui::DragFloat2("Linear Velocity (m/s)", vel, 0.1f)) {
        body->linearVelocity.x = vel[0];
        body->linearVelocity.y = vel[1];
      }

      // Angular Velocity
      float angVel = body->angularVelocity.z;
      if (ImGui::DragFloat("Angular Velocity (rad/s)", &angVel, 0.1f)) {
        body->angularVelocity.z = angVel;
      }
    }

    // Physics Material Properties
    ImGui::SliderFloat("Restitution", &body->restitution, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Friction", &body->friction, 0.0f, 1.0f, "%.2f");

    // Remove RigidBody Button
    if (ImGui::Button("Remove RigidBody", ImVec2(-1.0f, 0.0f))) {
      scene.physicsWorld.RemoveBody(body.get());
      entity.rigidBody.reset();
      ELYSIUM_INFO("Removed RigidBody from entity: {}", entity.name);
    }
  }
}

void EntityInspectorPanel::DrawCollidersComponent(EditorContext &context,
                                                 GameObject &entity) {
  (void)context;
  if (!entity.rigidBody || entity.rigidBody->colliders.empty())
    return;

  if (ImGui::CollapsingHeader("Colliders", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto &colliders = entity.rigidBody->colliders;
    for (size_t i = 0; i < colliders.size(); ++i) {
      auto &col = colliders[i];
      ImGui::PushID(static_cast<int>(i));

      if (col.type == ColliderType::Sphere) {
        ImGui::Text("Sphere Collider #%zu", i + 1);
        ImGui::DragFloat("Radius (m)", &col.radius, 0.02f, 0.01f, 50.0f);
      } else if (col.type == ColliderType::Box) {
        ImGui::Text("Box Collider #%zu", i + 1);
        float ext[2] = {col.halfExtents.x, col.halfExtents.y};
        if (ImGui::DragFloat2("Half Extents (m)", ext, 0.02f, 0.01f, 50.0f)) {
          col.halfExtents.x = ext[0];
          col.halfExtents.y = ext[1];
        }
      }

      ImGui::PopID();
      ImGui::Separator();
    }
  }
}

void EntityInspectorPanel::DrawSpriteRendererComponent(EditorContext &context,
                                                      GameObject &entity) {
  (void)context;
  auto *sprite = entity.GetComponent<SpriteRenderer>();
  if (!sprite)
    return;

  if (ImGui::CollapsingHeader("Sprite Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
    // Shape toggle
    const char *shapes[] = {"Box / Rectangle", "Circle / Sphere"};
    int currentShape = sprite->isCircle ? 1 : 0;
    if (ImGui::Combo("Shape", &currentShape, shapes, 2)) {
      sprite->isCircle = (currentShape == 1);
    }

    if (sprite->isCircle) {
      ImGui::DragFloat("Radius (m)", &sprite->radius, 0.02f, 0.01f, 50.0f);
    } else {
      float sz[2] = {sprite->size.x, sprite->size.y};
      if (ImGui::DragFloat2("Size (m)", sz, 0.02f, 0.01f, 50.0f)) {
        sprite->size.x = sz[0];
        sprite->size.y = sz[1];
      }
    }

    // Color picker
    float color[4] = {
        sprite->color.r / 255.0f,
        sprite->color.g / 255.0f,
        sprite->color.b / 255.0f,
        sprite->color.a / 255.0f,
    };
    if (ImGui::ColorEdit4("Tint Color", color)) {
      sprite->color = sf::Color(
          static_cast<uint8_t>(color[0] * 255.0f),
          static_cast<uint8_t>(color[1] * 255.0f),
          static_cast<uint8_t>(color[2] * 255.0f),
          static_cast<uint8_t>(color[3] * 255.0f));
    }
  }
}

void EntityInspectorPanel::DrawCustomComponents(EditorContext &context,
                                               GameObject &entity) {
  (void)context;
  if (entity.components.empty())
    return;

  if (ImGui::CollapsingHeader("Attached Components", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (size_t i = 0; i < entity.components.size(); ++i) {
      ImGui::PushID(static_cast<int>(i));
      ImGui::BulletText("Custom Component #%zu", i + 1);
      ImGui::PopID();
    }
  }
}

} // namespace Elysium
