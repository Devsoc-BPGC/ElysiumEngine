#include "panels/SceneHierarchyPanel.hpp"
#include "Collider.hpp"
#include "Log.h"
#include <algorithm>
#include "AudioSourceComponent.hpp"

namespace Elysium {

SceneHierarchyPanel::SceneHierarchyPanel() : EditorPanel("Scene Hierarchy") {}

void SceneHierarchyPanel::OnImGuiRender(EditorContext &context, Scene &scene,
                                        SimpleRenderer &renderer) {
  (void)renderer;

  if (ImGui::Begin("Scene Hierarchy", &isOpen)) {
    // Search input bar
    ImGui::Text("Filter:");
    ImGui::SameLine();
    m_filter.Draw("##HierarchyFilter", -1.0f);

    ImGui::Separator();

    // Add Entity button
    if (ImGui::Button("+ Add Entity", ImVec2(-1.0f, 0.0f))) {
      ImGui::OpenPopup("AddEntityPopup");
    }

    if (ImGui::BeginPopup("AddEntityPopup")) {
      if (ImGui::MenuItem("Empty Entity")) {
        auto entity = std::make_shared<GameObject>("Empty Entity");
        entity->position =
            Vec3(context.camera.center.x, context.camera.center.y, 0.0f);
        scene.AddGameObject(entity);
        context.selectedEntity = entity;
        ELYSIUM_INFO("Created new Empty Entity");
      }
      if (ImGui::MenuItem("Create Rigid Body")) {
        auto entity = std::make_shared<GameObject>("Rigid Body");
        entity->position =
            Vec3(context.camera.center.x, context.camera.center.y, 0.0f);
        auto &rb = entity->CreateRigidBody();
        scene.AddGameObject(entity);
        context.selectedEntity = entity;
      }
      if (ImGui::MenuItem("Create Audio Source")) {
        auto entity = std::make_shared<GameObject>("Audio Source");
        entity->position =
            Vec3(context.camera.center.x, context.camera.center.y, 0.0f);
        auto &as = entity->AddComponent<AudioSourceComponent>();
        scene.AddGameObject(entity);
        context.selectedEntity = entity;
        ELYSIUM_INFO("Added AudioSourceComponent to entity: {}", entity->GetComponent<AudioSourceComponent>() != nullptr);
      }

      ImGui::EndPopup();
    }

    ImGui::Spacing();

    // Entity list
    std::shared_ptr<GameObject> entityToDelete = nullptr;

    for (size_t i = 0; i < scene.objects.size(); ++i) {
      auto &obj = scene.objects[i];
      if (!obj)
        continue;

      if (!m_filter.PassFilter(obj->name.c_str()))
        continue;

      bool isSelected = (context.selectedEntity == obj);
      ImGuiTreeNodeFlags flags =
          (isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
          ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth |
          ImGuiTreeNodeFlags_NoTreePushOnOpen;

      std::string label = obj->name + "##" + std::to_string(static_cast<uint64_t>(obj->GetUUID()));
      ImGui::TreeNodeEx((void *)(uintptr_t)static_cast<uint64_t>(obj->GetUUID()), flags, "%s",
                        obj->name.c_str());

      if (ImGui::IsItemClicked()) {
        context.selectedEntity = obj;
      }

      // Context menu for each entity item
      std::string popupId = "EntityContextMenu##" + std::to_string(static_cast<uint64_t>(obj->GetUUID()));
      if (ImGui::BeginPopupContextItem(popupId.c_str())) {
        if (ImGui::MenuItem("Duplicate Entity")) {
          auto clone = std::make_shared<GameObject>(obj->name + " (Copy)");
          clone->position = Vec3(obj->position.x + 0.5f, obj->position.y + 0.5f,
                                 obj->position.z);
          clone->rotation = obj->rotation;
          clone->scale = obj->scale;
          if (obj->rigidBody) {
            auto &rb = clone->CreateRigidBody();
            rb.isStatic = obj->rigidBody->isStatic;
            rb.mass = obj->rigidBody->mass;
            rb.inverseMass = obj->rigidBody->inverseMass;
            rb.restitution = obj->rigidBody->restitution;
            rb.friction = obj->rigidBody->friction;
            for (const auto &col : obj->rigidBody->colliders) {
              rb.AddColliders(col);
            }
          }
          scene.AddGameObject(clone);
          context.selectedEntity = clone;
          ELYSIUM_INFO("Duplicated Entity: {}", clone->name);
        }

        if (ImGui::MenuItem("Delete Entity")) {
          entityToDelete = obj;
        }

        ImGui::EndPopup();
      }
    }

    // Deferred entity deletion to avoid modifying list during iteration
    if (entityToDelete) {
      if (context.selectedEntity == entityToDelete) {
        context.selectedEntity = nullptr;
      }
      if (entityToDelete->rigidBody) {
        scene.physicsWorld.RemoveBody(entityToDelete->rigidBody.get());
      }
      auto it =
          std::find(scene.objects.begin(), scene.objects.end(), entityToDelete);
      if (it != scene.objects.end()) {
        ELYSIUM_INFO("Deleted Entity: {}", entityToDelete->name);
        scene.objects.erase(it);
      }
    }

    // Right-click on blank window space
    if (ImGui::BeginPopupContextWindow(nullptr,
                                       ImGuiPopupFlags_MouseButtonRight |
                                           ImGuiPopupFlags_NoOpenOverItems)) {
      if (ImGui::MenuItem("Create Empty Entity")) {
        auto entity = std::make_shared<GameObject>("Empty Entity");
        entity->position =
            Vec3(context.camera.center.x, context.camera.center.y, 0.0f);
        scene.AddGameObject(entity);
        context.selectedEntity = entity;
      }
      if (ImGui::MenuItem("Create Ball")) {
        auto entity = std::make_shared<GameObject>("Ball");
        entity->position =
            Vec3(context.camera.center.x, context.camera.center.y, 0.0f);
        auto &rb = entity->CreateRigidBody();
        rb.AddColliders(Collider::CreateSphere(0.5f, 1.0f));
        scene.AddGameObject(entity);
        context.selectedEntity = entity;
      }
      ImGui::EndPopup();
    }
  }
  ImGui::End();
}

} // namespace Elysium
