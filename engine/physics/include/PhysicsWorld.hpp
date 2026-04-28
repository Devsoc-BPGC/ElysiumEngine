/**
 * @file PhysicsWorld.hpp
 * @brief Container class for managing rigid bodies, global forces, and world-level constraints.
 */

#ifndef PHYSICSWORLD_HPP
#define PHYSICSWORLD_HPP

#include <vector>
#include <algorithm>
#include "RigidBody.hpp"
#include "CoreMath.hpp"

/**
 * @class PhysicsWorld
 * @brief Manages the collection of rigid bodies and orchestrates the simulation steps.
 */
class PhysicsWorld {
public:
    /** @brief List of pointers to rigid bodies existing in this world simulation. */
    std::vector<RigidBody*> rigidBodies;

    /** @brief Global acceleration vector (e.g., Earth gravity at -9.81 on the Y-axis). */
    Vec3 gravity;

    /**
     * @brief Constructs a new Physics World.
     * @param gravityVector The initial global gravity force. Defaults to Earth-like gravity.
     */
    PhysicsWorld(Vec3 gravityVector = Vec3(0.0f, -9.81f, 0.0f))
        : gravity(gravityVector) {}

    /**
     * @brief Adds a RigidBody pointer to the simulation.
     * @note The world does not take ownership of the memory; the caller must manage deletion.
     * @param body Pointer to the RigidBody instance to simulate.
     */
    void AddBody(RigidBody* body) {
        rigidBodies.push_back(body);
    }

    /**
     * @brief Removes a RigidBody pointer from the simulation.
     * @param body Pointer to the RigidBody instance to remove.
     */
    void RemoveBody(RigidBody* body) {
        rigidBodies.erase(std::remove(rigidBodies.begin(), rigidBodies.end(), body), rigidBodies.end());
    }

    /**
     * @brief Advances the simulation by a fixed time step.
     * @param dt The time delta (change in time) in seconds.
     */
    void Step(float dt);

    /**
     * @brief Resets the force and torque accumulators for all bodies in the world.
     */
    void ClearForces();

    /**
     * @brief Performs simple static boundary collision and response.
     */
    void ResolveBoundaries() {
        float minX = 1.0f, maxX = 15.0f;
        float minY = 1.0f, maxY = 11.0f;
        float restitution = 0.8f;

        for (auto* body : rigidBodies) {
            bool collided = false;
            for (auto& col : body->colliders) {
                // Convert collider center to world coordinates
                Vec3 pos = body->LocalToGlobal(col.localCentroid);

                // --- Floor & Ceiling (Y-axis) ---
                if (pos.y + col.radius > maxY) { // Ceiling hit
                    body->position.y -= (pos.y + col.radius) - maxY;
                    if (body->linearVelocity.y > 0) body->linearVelocity.y *= -restitution;
                    collided = true;
                }
                else if (pos.y - col.radius < minY) { // Floor hit
                    body->position.y += minY - (pos.y - col.radius);
                    if (body->linearVelocity.y < 0) body->linearVelocity.y *= -restitution;
                    collided = true;
                }

                // --- Walls (X-axis) ---
                if (pos.x + col.radius > maxX) { // Right Wall hit
                    body->position.x -= (pos.x + col.radius) - maxX;
                    if (body->linearVelocity.x > 0) body->linearVelocity.x *= -restitution;
                    collided = true;
                }
                else if (pos.x - col.radius < minX) { // Left Wall hit
                    body->position.x += minX - (pos.x - col.radius);
                    if (body->linearVelocity.x < 0) body->linearVelocity.x *= -restitution;
                    collided = true;
                }
            }
            if (collided) {
                body->UpdateGlobalCentroidFromPosition();
            }
        }
    }
};

#endif
