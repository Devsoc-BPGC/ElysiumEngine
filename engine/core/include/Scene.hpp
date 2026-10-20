#ifndef SCENE_HPP
#define SCENE_HPP

#include "GameObject.hpp"
#include "PhysicsWorld.hpp"
#include "UUID.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

class Scene {
public:
  std::vector<std::shared_ptr<GameObject>> objects;
  std::unordered_map<Elysium::UUID, std::shared_ptr<GameObject>> m_EntityMap;
  PhysicsWorld physicsWorld;

  Scene();

  void AddGameObject(std::shared_ptr<GameObject> obj);
  GameObject *GetEntityByUUID(Elysium::UUID uuid);
  bool DestroyEntityByUUID(Elysium::UUID uuid);

  void Update(float dt);
};

namespace Elysium {
using Scene = ::Scene;
}

#endif
