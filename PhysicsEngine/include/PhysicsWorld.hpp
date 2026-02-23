#ifndef PHYSICSWORLD_HPP
#define PHYSICSWORLD_HPP

#include <vector>
#include "RigidBody.hpp"
#include "CoreMath.hpp"

class PhysicsWorld {
public:
    std::vector<RigidBody*> rigidBodies;
    Vec3 gravity;

    PhysicsWorld(Vec3 gravityVector = Vec3(0.0f, -9.81f, 0.0f))
        : gravity(gravityVector) {}

    void AddBody(RigidBody* body) {
        rigidBodies.push_back(body);
    }

    // This is the heart of your engine loop
    void Step(float dt);

    // Helper to clear forces manually if needed
    void ClearForces();
};

#endif
