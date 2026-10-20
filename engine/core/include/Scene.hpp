#ifndef SCENE_HPP
#define SCENE_HPP

#include "GameObject.hpp"
#include "PhysicsWorld.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

class Scene {
public:
  std::vector<std::shared_ptr<GameObject>> objects;
  PhysicsWorld physicsWorld;

  Scene() : physicsWorld(Vec3(0, -9.81f, 0)) {}

  void AddGameObject(std::shared_ptr<GameObject> obj) {
    objects.push_back(obj);
    if (obj->rigidBody) {
      physicsWorld.AddBody(obj->rigidBody.get());
    }
  }

  // Deep copy for Play mode: simulating the copy leaves this scene untouched.
  // Bodies are registered in the same order as here, so the copy simulates
  // exactly as this scene would.
  std::unique_ptr<Scene> Clone() const {
    auto copy = std::make_unique<Scene>();
    copy->physicsWorld = physicsWorld; // gravity, boundary, etc.
    copy->physicsWorld.rigidBodies.clear();

    std::unordered_map<const RigidBody *, RigidBody *> bodyCopies;
    for (const auto &obj : objects) {
      if (!obj)
        continue;
      auto objCopy = obj->Clone();
      if (obj->rigidBody) {
        bodyCopies[obj->rigidBody.get()] = objCopy->rigidBody.get();
      }
      copy->objects.push_back(std::move(objCopy));
    }
    for (const RigidBody *body : physicsWorld.rigidBodies) {
      auto it = bodyCopies.find(body);
      if (it != bodyCopies.end()) {
        copy->physicsWorld.AddBody(it->second);
      }
    }
    return copy;
  }

  void Update(float dt) {
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
};

#endif
