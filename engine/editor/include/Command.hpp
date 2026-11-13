#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "CoreMath.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"
#include "UUID.hpp"
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Elysium {

/**
 * @brief Base abstract interface for all undoable and redoable editor commands.
 */
class ICommand {
public:
  virtual ~ICommand() = default;
  virtual void Execute() = 0;
  virtual void Undo() = 0;
  virtual std::string GetName() const = 0;
};

/**
 * @brief Command recording a local transform modification on an entity.
 */
class TransformCommand : public ICommand {
public:
  TransformCommand(Scene &scene, UUID entityUUID,
                   const Vec3 &oldPos, const Quat &oldRot, const Vec3 &oldScale,
                   const Vec3 &newPos, const Quat &newRot, const Vec3 &newScale,
                   std::string actionName = "Transform")
      : m_Scene(scene), m_EntityUUID(entityUUID),
        m_OldPos(oldPos), m_OldRot(oldRot), m_OldScale(oldScale),
        m_NewPos(newPos), m_NewRot(newRot), m_NewScale(newScale),
        m_ActionName(std::move(actionName)) {}

  void Execute() override {
    Apply(m_NewPos, m_NewRot, m_NewScale);
  }

  void Undo() override {
    Apply(m_OldPos, m_OldRot, m_OldScale);
  }

  std::string GetName() const override { return m_ActionName; }

private:
  void Apply(const Vec3 &pos, const Quat &rot, const Vec3 &scale) {
    GameObject *entity = m_Scene.GetEntityByUUID(m_EntityUUID);
    if (!entity)
      return;

    entity->position = pos;
    entity->rotation = rot;
    entity->scale = scale;
    entity->GetTransform().SetLocalPosition(pos);
    entity->GetTransform().SetLocalRotation(rot);
    entity->GetTransform().SetLocalScale(scale);
    entity->SyncTransform();
  }

  Scene &m_Scene;
  UUID m_EntityUUID;
  Vec3 m_OldPos;
  Quat m_OldRot;
  Vec3 m_OldScale;
  Vec3 m_NewPos;
  Quat m_NewRot;
  Vec3 m_NewScale;
  std::string m_ActionName;
};

/**
 * @brief Templated command executing arbitrary entity property adjustments.
 */
template <typename T>
class PropertyChangeCommand : public ICommand {
public:
  PropertyChangeCommand(Scene &scene, UUID entityUUID,
                        T oldValue, T newValue,
                        std::function<void(GameObject &, const T &)> setter,
                        std::string actionName)
      : m_Scene(scene), m_EntityUUID(entityUUID),
        m_OldValue(std::move(oldValue)), m_NewValue(std::move(newValue)),
        m_Setter(std::move(setter)), m_ActionName(std::move(actionName)) {}

  void Execute() override {
    GameObject *entity = m_Scene.GetEntityByUUID(m_EntityUUID);
    if (entity && m_Setter) {
      m_Setter(*entity, m_NewValue);
    }
  }

  void Undo() override {
    GameObject *entity = m_Scene.GetEntityByUUID(m_EntityUUID);
    if (entity && m_Setter) {
      m_Setter(*entity, m_OldValue);
    }
  }

  std::string GetName() const override { return m_ActionName; }

private:
  Scene &m_Scene;
  UUID m_EntityUUID;
  T m_OldValue;
  T m_NewValue;
  std::function<void(GameObject &, const T &)> m_Setter;
  std::string m_ActionName;
};

/**
 * @brief Command recording a parent-child hierarchy modification.
 */
class ReparentEntityCommand : public ICommand {
public:
  ReparentEntityCommand(Scene &scene, UUID childUUID, UUID oldParentUUID, UUID newParentUUID,
                        std::string actionName = "Reparent Entity")
      : m_Scene(scene), m_ChildUUID(childUUID),
        m_OldParentUUID(oldParentUUID), m_NewParentUUID(newParentUUID),
        m_ActionName(std::move(actionName)) {}

  void Execute() override {
    GameObject *child = m_Scene.GetEntityByUUID(m_ChildUUID);
    if (child) {
      child->SetParent(m_NewParentUUID);
    }
  }

  void Undo() override {
    GameObject *child = m_Scene.GetEntityByUUID(m_ChildUUID);
    if (child) {
      child->SetParent(m_OldParentUUID);
    }
  }

  std::string GetName() const override { return m_ActionName; }

private:
  Scene &m_Scene;
  UUID m_ChildUUID;
  UUID m_OldParentUUID;
  UUID m_NewParentUUID;
  std::string m_ActionName;
};

/**
 * @brief Command recording entity creation in a scene.
 */
class CreateEntityCommand : public ICommand {
public:
  CreateEntityCommand(Scene &scene, std::shared_ptr<GameObject> entity,
                      std::string actionName = "")
      : m_Scene(scene), m_Entity(std::move(entity)) {
    if (m_Entity) {
      m_EntityUUID = m_Entity->GetUUID();
      m_EntityName = actionName.empty() ? ("Create Entity '" + m_Entity->name + "'") : actionName;
    }
  }

  void Execute() override {
    if (m_Entity && !m_Scene.GetEntityByUUID(m_EntityUUID)) {
      m_Scene.AddGameObject(m_Entity);
    }
  }

  void Undo() override {
    if (m_EntityUUID != 0) {
      m_Scene.DestroyEntityByUUID(m_EntityUUID);
    }
  }

  std::string GetName() const override { return m_EntityName; }

private:
  Scene &m_Scene;
  std::shared_ptr<GameObject> m_Entity;
  UUID m_EntityUUID{0};
  std::string m_EntityName;
};

/**
 * @brief Command recording entity deletion with subtree preservation.
 */
class DeleteEntityCommand : public ICommand {
public:
  struct DeletedEntityInfo {
    std::shared_ptr<GameObject> entity;
    UUID parentHandle{0};
  };

  DeleteEntityCommand(Scene &scene, const std::shared_ptr<GameObject> &entity,
                      std::string actionName = "")
      : m_Scene(scene) {
    if (entity) {
      m_RootUUID = entity->GetUUID();
      m_RootName = actionName.empty() ? ("Delete Entity '" + entity->name + "'") : actionName;
      CollectSubtree(entity);
    }
  }

  void Execute() override {
    if (m_RootUUID != 0) {
      m_Scene.DestroyEntityByUUID(m_RootUUID);
    }
  }

  void Undo() override {
    // 1. Re-add all entities back into scene
    for (auto &info : m_Entities) {
      if (!info.entity)
        continue;
      m_Scene.AddGameObject(info.entity);
    }

    // 2. Re-establish parent hierarchy
    for (auto &info : m_Entities) {
      if (!info.entity)
        continue;
      if (info.parentHandle != 0) {
        info.entity->SetParent(info.parentHandle);
      }
    }
  }

  std::string GetName() const override { return m_RootName; }

private:
  void CollectSubtree(const std::shared_ptr<GameObject> &root) {
    if (!root)
      return;
    DeletedEntityInfo info;
    info.entity = root;
    info.parentHandle = root->GetTransform().ParentHandle;
    m_Entities.push_back(info);

    for (UUID childUUID : root->GetTransform().ChildrenHandles) {
      auto it = m_Scene.m_EntityMap.find(childUUID);
      if (it != m_Scene.m_EntityMap.end()) {
        CollectSubtree(it->second);
      }
    }
  }

  Scene &m_Scene;
  UUID m_RootUUID{0};
  std::string m_RootName;
  std::vector<DeletedEntityInfo> m_Entities;
};

} // namespace Elysium

#endif // COMMAND_HPP
