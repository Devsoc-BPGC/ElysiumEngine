#pragma once

#include "GameObject.hpp"
#include "Scene.hpp"
#include "UUID.hpp"

namespace Elysium {

template <typename T = GameObject> class EntityHandle {
public:
  EntityHandle() : m_UUID(0) {}
  EntityHandle(UUID uuid) : m_UUID(uuid) {}
  EntityHandle(const GameObject *entity)
      : m_UUID(entity ? entity->GetUUID() : UUID(0)) {}

  UUID GetUUID() const noexcept { return m_UUID; }
  bool IsValid() const noexcept { return static_cast<uint64_t>(m_UUID) != 0; }
  explicit operator bool() const noexcept { return IsValid(); }

  bool operator==(const EntityHandle &other) const noexcept {
    return m_UUID == other.m_UUID;
  }
  bool operator!=(const EntityHandle &other) const noexcept {
    return m_UUID != other.m_UUID;
  }

  GameObject *Resolve(Scene *scene) const {
    if (!scene || !IsValid()) {
      return nullptr;
    }
    return scene->GetEntityByUUID(m_UUID);
  }

  T *ResolveComponent(Scene *scene) const {
    GameObject *entity = Resolve(scene);
    if (!entity) {
      return nullptr;
    }

    if constexpr (std::is_same_v<T, GameObject>) {
      return entity;
    } else {
      return entity->template GetComponent<T>();
    }
  }

private:
  UUID m_UUID{0};
};

} // namespace Elysium
