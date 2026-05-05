#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>
#include <memory>
#include "GameObject.hpp"
#include "PhysicsWorld.hpp"

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

    void Update(float dt) {
        // 1. Update components
        for (auto& obj : objects) {
            if (obj->active) obj->Update(dt);
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
        for (auto& obj : objects) {
            obj->SyncPhysics();
        }
    }
};

#endif
