#include "panels/SceneHierarchyPanel.hpp"
#include "Collider.hpp"
#include "Log.h"
#include <algorithm>
#include "AudioSourceComponent.hpp"

namespace Elysium {

static constexpr const char *SCENE_HIERARCHY_ENTITY = "SCENE_HIERARCHY_ENTITY";

SceneHierarchyPanel::SceneHierarchyPanel() : EditorPanel("Scene Hierarchy") {}

void SceneHierarchyPanel::DrawEntityNode(EditorContext &context, Scene &scene,
                                         const std::shared_ptr<GameObject> &obj,
                                         std::shared_ptr<GameObject> &entityToDelete) {
  if (!obj)
    return;

  bool hasChildren = !obj->GetTransform().ChildrenHandles.empty();
  bool isSelected = (context.selectedEntity == obj);

  ImGuiTreeNodeFlags flags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
                             ImGuiTreeNodeFlags_OpenOnArrow |
                             ImGuiTreeNodeFlags_SpanAvailWidth;
  if (!hasChildren) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }

  std::string label = obj->name + "##" + std::to_string(static_cast<uint64_t>(obj->GetUUID()));
  bool nodeOpen = ImGui::TreeNodeEx((void *)(uintptr_t)static_cast<uint64_t>(obj->GetUUID()),
                                    flags, "%s", obj->name.c_str());

  if (ImGui::IsItemClicked()) {
    context.selectedEntity = obj;
  }

  // Drag Source: Allow dragging this entity to reparent
  if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
    uint64_t rawUUID = static_cast<uint64_t>(obj->GetUUID());
    ImGui::SetDragDropPayload(SCENE_HIERARCHY_ENTITY, &rawUUID, sizeof(uint64_t));
    ImGui::Text("Attach '%s'", obj->name.c_str());
    ImGui::EndDragDropSource();
  }

  // Drag Target on Node: Reparent dragged entity to this entity
  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(SCENE_HIERARCHY_ENTITY)) {
      if (payload->DataSize == sizeof(uint64_t)) {
        uint64_t droppedRaw = *static_cast<const uint64_t *>(payload->Data);
        Elysium::UUID droppedUUID(droppedRaw);
        GameObject *draggedEntity = scene.GetEntityByUUID(droppedUUID);
        if (draggedEntity && draggedEntity->GetUUID() != obj->GetUUID() &&
            !draggedEntity->IsParentOf(obj->GetUUID())) {
          draggedEntity->SetParent(obj->GetUUID());
          ELYSIUM_INFO("Reparented entity '{}' under '{}'", draggedEntity->name, obj->name);
        }
      }
    }
    ImGui::EndDragDropTarget();
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

    if (obj->GetTransform().ParentHandle != 0) {
      if (ImGui::MenuItem("Unparent Entity")) {
        obj->SetParent(0);
        ELYSIUM_INFO("Unparented Entity: {}", obj->name);
      }
    }

    if (ImGui::MenuItem("Delete Entity")) {
      entityToDelete = obj;
    }

    ImGui::EndPopup();
  }

  // Recursively render children if node is opened
  if (nodeOpen) {
    auto children = obj->GetTransform().ChildrenHandles;
    for (Elysium::UUID childUUID : children) {
      GameObject *child = scene.GetEntityByUUID(childUUID);
      if (child) {
        // Find shared_ptr to pass down
        auto it = scene.m_EntityMap.find(childUUID);
        if (it != scene.m_EntityMap.end()) {
          DrawEntityNode(context, scene, it->second, entityToDelete);
        }
      }
    }
    ImGui::TreePop();
  }
}

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

    // Entity tree list
    std::shared_ptr<GameObject> entityToDelete = nullptr;

    bool isFiltering = m_filter.IsActive();
    if (isFiltering) {
      // In filter mode, show all matching entities flatly
      for (size_t i = 0; i < scene.objects.size(); ++i) {
        auto &obj = scene.objects[i];
        if (!obj || !m_filter.PassFilter(obj->name.c_str()))
          continue;

        bool isSelected = (context.selectedEntity == obj);
        ImGuiTreeNodeFlags flags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth |
                                   ImGuiTreeNodeFlags_NoTreePushOnOpen;
        ImGui::TreeNodeEx((void *)(uintptr_t)static_cast<uint64_t>(obj->GetUUID()), flags, "%s",
                          obj->name.c_str());
        if (ImGui::IsItemClicked()) {
          context.selectedEntity = obj;
        }
      }
    } else {
      // Hierarchy tree mode: only start from root entities (ParentHandle == 0)
      for (size_t i = 0; i < scene.objects.size(); ++i) {
        auto &obj = scene.objects[i];
        if (!obj || obj->GetTransform().ParentHandle != 0)
          continue;

        DrawEntityNode(context, scene, obj, entityToDelete);
      }
    }

    // Deferred entity deletion to avoid modifying list during iteration
    if (entityToDelete) {
      if (context.selectedEntity == entityToDelete) {
        context.selectedEntity = nullptr;
      }
      ELYSIUM_INFO("Deleted Entity: {}", entityToDelete->name);
      scene.DestroyEntityByUUID(entityToDelete->GetUUID());
    }

    // Drag-and-drop into empty hierarchy window space: Unparent back to root
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.y > 0.0f) {
      ImGui::InvisibleButton("##HierarchyEmptySpaceDropTarget", ImVec2(-1.0f, std::max(avail.y, 40.0f)));
      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(SCENE_HIERARCHY_ENTITY)) {
          if (payload->DataSize == sizeof(uint64_t)) {
            uint64_t droppedRaw = *static_cast<const uint64_t *>(payload->Data);
            Elysium::UUID droppedUUID(droppedRaw);
            GameObject *draggedEntity = scene.GetEntityByUUID(droppedUUID);
            if (draggedEntity && draggedEntity->GetTransform().ParentHandle != 0) {
              draggedEntity->SetParent(0);
              ELYSIUM_INFO("Detached entity '{}' to root level", draggedEntity->name);
            }
          }
        }
        ImGui::EndDragDropTarget();
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
