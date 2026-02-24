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

    void Step(float dt);

    void ClearForces();

    void ResolveBoundaries() {
        float minX = 1.0f, maxX = 15.0f;
        float minY = 1.0f, maxY = 11.0f;
        float restitution = 0.8f;

        for (auto* body : rigidBodies) {
            for (auto& col : body->colliders) {
                Vec3 pos = body->LocalToGlobal(col.localCentroid);

                // --- Floor & Ceiling (Y-axis) ---
                if (pos.y + col.radius > maxY) { // Hit Floor
                    body->position.y -= (pos.y + col.radius) - maxY;
                    if (body->linearVelocity.y > 0) body->linearVelocity.y *= -restitution;
                }
                else if (pos.y - col.radius < minY) { // Hit Ceiling
                    body->position.y += minY - (pos.y - col.radius);
                    if (body->linearVelocity.y < 0) body->linearVelocity.y *= -restitution;
                }

                // --- Walls (X-axis) ---
                if (pos.x + col.radius > maxX) { // Hit Right Wall
                    body->position.x -= (pos.x + col.radius) - maxX;
                    if (body->linearVelocity.x > 0) body->linearVelocity.x *= -restitution;
                }
                else if (pos.x - col.radius < minX) { // Hit Left Wall
                    body->position.x += minX - (pos.x - col.radius);
                    if (body->linearVelocity.x < 0) body->linearVelocity.x *= -restitution;
                }
            }
        }
    }
};

#endif
