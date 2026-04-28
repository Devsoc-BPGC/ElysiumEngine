/**
 * @file PhysicsWorld.cpp
 * @brief Implementation of the physics simulation loop and force management.
 */

#include "PhysicsWorld.hpp"
#include "BroadPhase.hpp"
#include "NarrowPhase.hpp"

/**
 * @brief Advances the physics simulation by one time step.
 * * The execution flow follows a standard semi-implicit Euler pipeline:
 * 1. **Force Application**: Applies global gravity to all non-static bodies.
 * 2. **Integration**: Updates velocities and positions based on accumulated forces.
 * 3. **Broad Phase**: Prunes objects that are too far apart to collide, returning potential pairs.
 * 4. **Narrow Phase**: Performs detailed collision checks and resolves impulses between bodies.
 * 5. **Constraint Resolution**: Keeps bodies within the world boundaries (box constraints).
 * * @param dt The time delta for the current frame.
 */
void PhysicsWorld::Step(float dt) {
    // 1. Apply Gravity
    // Computes F = m * g and applies it at the center of mass.
    for (auto* body : rigidBodies) {
        if (body->inverseMass > 0.0f) {
            float mass = 1.0f / body->inverseMass;
            body->ApplyForce(gravity * mass, body->globalCentroid);
        }
    }

    // 2. Integration
    // Moves bodies based on the forces applied in step 1.
    for (auto* body : rigidBodies) {
        body->Integrate(dt);
    }

    // 3. Broad Phase
    // Optimization step to find potential collision pairs using AABBs.
    std::vector<CollisionPair> candidates = BroadPhase::GeneratePairs(rigidBodies);

    // 4. Narrow Phase (Body vs Body)
    // Detailed geometric intersection tests and impulse resolution.
    for (auto& pair : candidates) {
        NarrowPhase::ResolveCollision(pair.A, pair.B);
    }

    // 5. Boundary Resolution
    // Final check to ensure balls stay within the simulation box after collisions.
    ResolveBoundaries();

    // Secondary Broad Phase check (often used for debug visualization or logging)
    std::vector<CollisionPair> pairs = BroadPhase::GeneratePairs(rigidBodies);

    // Temporary Debug Hook
    if (!pairs.empty()) {
        // std::cout << "Broad Phase found " << pairs.size() << " potential collisions!" << std::endl;
    }
}

/**
 * @brief Manually clears all accumulated linear forces and torques.
 * * Usually called if the user wants to reset the simulation state or
 * manually manage force accumulation outside of the Step() function.
 */
void PhysicsWorld::ClearForces() {
    for (auto* body : rigidBodies) {
        body->forceAccumulator.Zero();
        body->torqueAccumulator.Zero();
    }
}
