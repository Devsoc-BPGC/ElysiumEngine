#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include "Component.hpp"
#include "../../PhysicsEngine/include/RigidBody.hpp"
#include "../../PhysicsEngine/include/CoreMath.hpp"

class GameObject {
public:
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

    GameObject(const std::string& name = "New GameObject")
        : name(name), position(0, 0, 0), rotation(Quat()), scale(1, 1, 1) {}

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args) {
        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        comp->gameObject = this;
        T& ref = *comp;
        components.push_back(std::move(comp));
        return ref;
    }

    /**
     * @brief Creates and attaches a RigidBody to this GameObject.
     */
    RigidBody& CreateRigidBody() {
        rigidBody = std::make_unique<RigidBody>();
        rigidBody->position = position;
        rigidBody->orientation = rotation.ToMatrix();
        return *rigidBody;
    }

    void Update(float dt) {
        for (auto& comp : components) {
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
