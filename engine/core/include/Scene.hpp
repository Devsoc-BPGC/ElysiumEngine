#ifndef SCENE_HPP
#define SCENE_HPP

#include "GameObject.hpp"
#include "PhysicsWorld.hpp"
#include "UUID.hpp"
#include "Texture.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

class Scene {
public:
  std::vector<std::shared_ptr<GameObject>> objects;
  std::unordered_map<Elysium::UUID, std::shared_ptr<GameObject>> m_EntityMap;
  std::vector<std::shared_ptr<const Texture>> textures;
  PhysicsWorld physicsWorld;

  Scene();

  void AddGameObject(std::shared_ptr<GameObject> obj);
  GameObject *GetEntityByUUID(Elysium::UUID uuid);
  bool DestroyEntityByUUID(Elysium::UUID uuid);
  void Clear();

  void AddTexture(std::shared_ptr<const Texture> texture);

  // Deep copy for Play mode: simulating the copy leaves this scene untouched
  std::unique_ptr<Scene> Clone() const;

  void Update(float dt);
};

namespace Elysium {
using Scene = ::Scene;
}

#endif
