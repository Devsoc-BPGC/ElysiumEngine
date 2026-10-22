#include "GameObject.hpp"
#include "Log.h"

GameObject::GameObject(const std::string &initialName)
    : m_UUID(), name(initialName), position(0, 0, 0), rotation(Quat()),
      scale(1, 1, 1) {
  ELYSIUM_CORE_TRACE("GameObject created: '{}' [UUID: {}]", name,
                     static_cast<uint64_t>(m_UUID));
}

GameObject::GameObject(Elysium::UUID uuid, const std::string &initialName)
    : m_UUID(uuid), name(initialName), position(0, 0, 0), rotation(Quat()),
      scale(1, 1, 1) {
  ELYSIUM_CORE_TRACE("GameObject created with explicit UUID: '{}' [UUID: {}]",
                     name, static_cast<uint64_t>(m_UUID));
}
