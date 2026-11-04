#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include "Component.hpp"
#include "CoreMath.hpp"
#include "RigidBody.hpp"
#include "TransformComponent.hpp"
#include "UUID.hpp"
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

class Scene;

class GameObject {
public:
  Elysium::UUID m_UUID;
  std::string name;
  bool active = true;

  // Scene pointer
  Scene *m_Scene = nullptr;

  // Transform Component
  TransformComponent m_Transform;

  // Transform data (kept synchronized with m_Transform)
  Vec3 position;
  Quat rotation;
  Vec3 scale;

  // Physics component (managed specifically for speed and scene integration)
  std::unique_ptr<RigidBody> rigidBody;

  // Generic components
  std::vector<std::unique_ptr<Component>> components;

  GameObject(const std::string &initialName = "New GameObject");
  GameObject(Elysium::UUID uuid, const std::string &initialName = "New GameObject");

  Elysium::UUID GetUUID() const noexcept { return m_UUID; }
  Scene *GetScene() const noexcept { return m_Scene; }
  void SetScene(Scene *scene) noexcept { m_Scene = scene; }

  // Transform getters & setters
  TransformComponent &GetTransform() noexcept { return m_Transform; }
  const TransformComponent &GetTransform() const noexcept { return m_Transform; }

  Vec3 GetLocalPosition() const { return m_Transform.GetLocalPosition(); }
  Vec3 GetWorldPosition() const { return m_Transform.GetWorldPosition(); }
  Quat GetLocalRotation() const { return m_Transform.GetLocalRotation(); }
  Quat GetWorldRotation() const { return m_Transform.GetWorldRotation(); }
  Vec3 GetLocalScale() const { return m_Transform.GetLocalScale(); }
  Vec3 GetWorldScale() const { return m_Transform.GetWorldScale(); }
  const Mat4 &GetWorldTransform() const { return m_Transform.GetWorldMatrix(); }

  // Hierarchy methods
  void SetParent(Elysium::UUID parentUUID);
  void RemoveChild(Elysium::UUID childUUID);
  bool IsParentOf(Elysium::UUID entityUUID) const;

  template <typename T, typename... Args> T &AddComponent(Args &&...args) {
    auto comp = std::make_unique<T>(std::forward<Args>(args)...);
    comp->gameObject = this;
    T &ref = *comp;
    components.push_back(std::move(comp));
    return ref;
  }

  template <typename T> T *GetComponent() {
    if constexpr (std::is_same_v<T, TransformComponent>) {
      return &m_Transform;
    }
    for (auto &comp : components) {
      if (auto casted = dynamic_cast<T *>(comp.get()))
        return casted;
    }
    return nullptr;
  }

  template <typename T> bool HasComponent() const {
    if constexpr (std::is_same_v<T, TransformComponent>) {
      return true;
    }
    for (const auto &comp : components) {
      if (dynamic_cast<const T *>(comp.get()))
        return true;
    }
    return false;
  }

  /**
   * @brief Creates and attaches a RigidBody to this GameObject.
   */
  RigidBody &CreateRigidBody() {
    rigidBody = std::make_unique<RigidBody>();
    rigidBody->position = GetWorldPosition();
    rigidBody->orientation = GetWorldRotation().ToMatrix();
    return *rigidBody;
  }

  /**
   * @brief Deep-copies this GameObject: transform, RigidBody and components.
   * @note The copy keeps this object's UUID so the two can be matched up.
   */
  std::shared_ptr<GameObject> Clone() const;

  void Update(float dt) {
    for (auto &comp : components) {
      comp->Update(dt);
    }
  }

  /**
   * @brief Syncs the GameObject transform with the Physics RigidBody.
   */
  void SyncPhysics();

  /**
   * @brief Syncs the Physics RigidBody with the GameObject transform.
   */
  void SyncTransform();
};

namespace Elysium {
using GameObject = ::GameObject;
}

#endif // GAMEOBJECT_HPP
