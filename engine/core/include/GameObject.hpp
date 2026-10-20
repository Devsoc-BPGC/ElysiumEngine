#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include "Component.hpp"
#include "CoreMath.hpp"
#include "RigidBody.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

class GameObject {
public:
  inline static uint32_t s_NextID = 1;
  uint32_t id = s_NextID++;
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

  GameObject(const std::string &initialName = "New GameObject")
      : name(initialName), position(0, 0, 0), rotation(Quat()), scale(1, 1, 1) {
  }

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
   * @note The copy keeps this object's id so the two can be matched up.
   */
  std::shared_ptr<GameObject> Clone() const {
    auto copy = std::make_shared<GameObject>(name);
    copy->id = id;
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

#endif
