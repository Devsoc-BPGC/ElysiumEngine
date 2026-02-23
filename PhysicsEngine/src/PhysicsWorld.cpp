#include "../include/PhysicsWorld.hpp"

void PhysicsWorld::Step(float dt) {
    // 1. Apply Global Forces
    for (auto* body : rigidBodies) {
        if (body->inverseMass > 0.0f) {
            float mass = 1.0f / body->inverseMass;

            // Force = mass * acceleration
            Vec3 gravityForce = gravity * mass;

            // Apply gravity at the center of mass (globalCentroid)
            // to avoid inducing rotation from gravity itself.
            body->ApplyForce(gravityForce, body->globalCentroid);
        }
    }

    // 2. INTEGRATION PHASE
    // This moves the bodies based on the forces applied above
    for (auto* body : rigidBodies) {
        body->Integrate(dt);
    }

    // 3. TODO: Collision Detection (Narrow phase / Broad phase)
    // 4. TODO: Constraint Solver (Impulse resolution)
}

void PhysicsWorld::ClearForces() {
    for (auto* body : rigidBodies) {
        body->forceAccumulator.Zero();
        body->torqueAccumulator.Zero();
    }
}
