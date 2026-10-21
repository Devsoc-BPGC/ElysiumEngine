#include "Scene.hpp"
#include "Log.h"
#include <algorithm>

Scene::Scene() : physicsWorld(Vec3(0, -9.81f, 0)) {}

void Scene::AddGameObject(std::shared_ptr<GameObject> obj) {
  if (!obj)
    return;

  objects.push_back(obj);
  m_EntityMap[obj->GetUUID()] = obj;

  if (obj->rigidBody) {
    physicsWorld.AddBody(obj->rigidBody.get());
  }

  ELYSIUM_CORE_TRACE("Entity registered to Scene: '{}' [UUID: {}]", obj->name,
                     static_cast<uint64_t>(obj->GetUUID()));
}

GameObject *Scene::GetEntityByUUID(Elysium::UUID uuid) {
  auto it = m_EntityMap.find(uuid);
  if (it != m_EntityMap.end()) {
    return it->second.get();
  }
  return nullptr;
}

bool Scene::DestroyEntityByUUID(Elysium::UUID uuid) {
  auto it = m_EntityMap.find(uuid);
  if (it == m_EntityMap.end()) {
    ELYSIUM_CORE_WARN("DestroyEntityByUUID: Entity with UUID {} not found",
                      static_cast<uint64_t>(uuid));
    return false;
  }

  std::shared_ptr<GameObject> entity = it->second;
  if (entity) {
    entity->active = false;
    if (entity->rigidBody) {
      physicsWorld.RemoveBody(entity->rigidBody.get());
    }
  }

  m_EntityMap.erase(it);

  auto vecIt = std::find_if(objects.begin(), objects.end(),
                            [uuid](const std::shared_ptr<GameObject> &o) {
                              return o && o->GetUUID() == uuid;
                            });
  if (vecIt != objects.end()) {
    objects.erase(vecIt);
  }

  ELYSIUM_CORE_INFO("Destroyed entity [UUID: {}]", static_cast<uint64_t>(uuid));
  return true;
}

void Scene::Update(float dt) {
  // 1. Update components
  for (auto &obj : objects) {
    if (obj->active)
      obj->Update(dt);
  }

  // 2. Cleanup inactive objects
  auto it = objects.begin();
  while (it != objects.end()) {
    if (!(*it)->active) {
      if ((*it)->rigidBody) {
        physicsWorld.RemoveBody((*it)->rigidBody.get());
      }
      m_EntityMap.erase((*it)->GetUUID());
      it = objects.erase(it);
    } else {
      ++it;
    }
  }

  // 3. Step physics
  physicsWorld.Step(dt);

  // 4. Sync game objects with physics results
  for (auto &obj : objects) {
    obj->SyncPhysics();
  }
}
