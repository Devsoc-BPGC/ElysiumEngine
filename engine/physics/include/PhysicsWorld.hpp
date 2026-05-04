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

    /** @brief Boundary box center. */
    Vec3 boundaryCenter = Vec3(8.0f, 6.0f, 0.0f);
    /** @brief Boundary box half-extents. */
    Vec3 boundaryHalfExtents = Vec3(5.0f, 5.0f, 0.0f);
    /** @brief Boundary box rotation in radians. */
    float boundaryRotation = 0.0f;

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
        float restitution = 0.8f;
        Mat3 rot = Mat3::RotationZ(boundaryRotation);
        Mat3 rotInv = rot.Transposed();

        for (auto* body : rigidBodies) {
            bool collided = false;
            for (auto& col : body->colliders) {
                // Convert collider center to world coordinates
                Vec3 globalPos = body->LocalToGlobal(col.localCentroid);

                // Transform to boundary-local space
                Vec3 localPos = rotInv * (globalPos - boundaryCenter);
                Vec3 localVel = rotInv * body->linearVelocity;

                bool subCollided = false;

                // --- Floor & Ceiling (Local Y-axis) ---
                if (localPos.y + col.radius > boundaryHalfExtents.y) { // Ceiling hit
                    localPos.y -= (localPos.y + col.radius) - boundaryHalfExtents.y;
                    if (localVel.y > 0) localVel.y *= -restitution;
                    subCollided = true;
                }
                else if (localPos.y - col.radius < -boundaryHalfExtents.y) { // Floor hit
                    localPos.y += -boundaryHalfExtents.y - (localPos.y - col.radius);
                    if (localVel.y < 0) localVel.y *= -restitution;
                    subCollided = true;
                }

                // --- Walls (Local X-axis) ---
                if (localPos.x + col.radius > boundaryHalfExtents.x) { // Right Wall hit
                    localPos.x -= (localPos.x + col.radius) - boundaryHalfExtents.x;
                    if (localVel.x > 0) localVel.x *= -restitution;
                    subCollided = true;
                }
                else if (localPos.x - col.radius < -boundaryHalfExtents.x) { // Left Wall hit
                    localPos.x += -boundaryHalfExtents.x - (localPos.x - col.radius);
                    if (localVel.x < 0) localVel.x *= -restitution;
                    subCollided = true;
                }

                if (subCollided) {
                    // Transform back to global space
                    body->position = boundaryCenter + (rot * localPos) - body->orientation.ToQuat().RotateVector(col.localCentroid);
                    body->linearVelocity = rot * localVel;
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
