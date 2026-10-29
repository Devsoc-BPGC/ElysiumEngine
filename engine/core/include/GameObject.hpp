#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include "Component.hpp"
#include "CoreMath.hpp"
#include "RigidBody.hpp"
#include "UUID.hpp"
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

class GameObject {
public:
  Elysium::UUID m_UUID;
  std::string name;
  bool active = true;

  // Transform data
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

  template <typename T, typename... Args> T &AddComponent(Args &&...args) {
    auto comp = std::make_unique<T>(std::forward<Args>(args)...);
    comp->gameObject = this;
    T &ref = *comp;
    components.push_back(std::move(comp));
    return ref;
  }

  template <typename T> T *GetComponent() {
    for (auto &comp : components) {
      if (auto casted = dynamic_cast<T *>(comp.get()))
        return casted;
    }
    return nullptr;
  }

  template <typename T> bool HasComponent() const {
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
    rigidBody->position = position;
    rigidBody->orientation = rotation.ToMatrix();
    return *rigidBody;
  }

  /**
   * @brief Deep-copies this GameObject: transform, RigidBody and components.
   * @note The copy keeps this object's UUID so the two can be matched up.
   */
  std::shared_ptr<GameObject> Clone() const {
    auto copy = std::make_shared<GameObject>(m_UUID, name);
    copy->active = active;
    copy->position = position;
    copy->rotation = rotation;
    copy->scale = scale;
    if (rigidBody) {
      copy->rigidBody = std::make_unique<RigidBody>(*rigidBody);
    }
    for (const auto &comp : components) {
      if (auto compCopy = comp->Clone()) {
        compCopy->gameObject = copy.get();
        copy->components.push_back(std::move(compCopy));
      }
    }
    // Once the copy is complete, let components rebind anything else that
    // still refers to the original (e.g. a script's Entity handle)
    for (auto &comp : copy->components) {
      comp->OnCloned(copy);
    }
    return copy;
  }

  void Update(float dt) {
    for (auto &comp : components) {
      comp->Update(dt);
    }
  }

  /**
   * @brief Syncs the GameObject transform with the Physics RigidBody.
   */
  void SyncPhysics() {
    if (rigidBody) {
      position = rigidBody->position;
      rotation = rigidBody->orientation.ToQuat();
    }
  }

  /**
   * @brief Syncs the Physics RigidBody with the GameObject transform.
   */
  void SyncTransform() {
    if (rigidBody) {
      rigidBody->position = position;
      rigidBody->orientation = rotation.ToMatrix();
      rigidBody->UpdateGlobalCentroidFromPosition();
    }
  }
};

namespace Elysium {
using GameObject = ::GameObject;
}

#endif
