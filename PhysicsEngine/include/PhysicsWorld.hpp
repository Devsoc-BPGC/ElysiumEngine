/**
 * @file PhysicsWorld.hpp
 * @brief Container class for managing rigid bodies, global forces, and world-level constraints.
 */

#ifndef PHYSICSWORLD_HPP
#define PHYSICSWORLD_HPP

#include <vector>
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
     * * This function checks every collider against a hard-coded bounding box.
     * If a collider penetrates a boundary, it performs two actions:
     * 1. **Position Correction**: Instantly moves the body back within bounds to prevent sinking.
     * 2. **Velocity Reflection**: Flips the velocity component and applies a coefficient of restitution.
     * * @note This uses a simple "AABB vs Point-Radius" logic for the screen/world edges.
     */
    void ResolveBoundaries() {
        float minX = 1.0f, maxX = 15.0f;
        float minY = 1.0f, maxY = 11.0f;
        float restitution = 0.8f;

        for (auto* body : rigidBodies) {
            for (auto& col : body->colliders) {
                // Convert collider center to world coordinates
                Vec3 pos = body->LocalToGlobal(col.localCentroid);

                // --- Floor & Ceiling (Y-axis) ---
                if (pos.y + col.radius > maxY) { // Ceiling hit
                    body->position.y -= (pos.y + col.radius) - maxY;
                    if (body->linearVelocity.y > 0) body->linearVelocity.y *= -restitution;
                }
                else if (pos.y - col.radius < minY) { // Floor hit
                    body->position.y += minY - (pos.y - col.radius);
                    if (body->linearVelocity.y < 0) body->linearVelocity.y *= -restitution;
                }

                // --- Walls (X-axis) ---
                if (pos.x + col.radius > maxX) { // Right Wall hit
                    body->position.x -= (pos.x + col.radius) - maxX;
                    if (body->linearVelocity.x > 0) body->linearVelocity.x *= -restitution;
                }
                else if (pos.x - col.radius < minX) { // Left Wall hit
                    body->position.x += minX - (pos.x - col.radius);
                    if (body->linearVelocity.x < 0) body->linearVelocity.x *= -restitution;
                }
            }
        }
    }
};

#endif
