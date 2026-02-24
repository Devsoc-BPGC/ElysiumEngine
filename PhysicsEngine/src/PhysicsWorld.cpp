#include "PhysicsWorld.hpp"
#include "BroadPhase.hpp"
#include "NarrowPhase.hpp"
// #include <iostream>

void PhysicsWorld::Step(float dt) {
    // 1. Apply Gravity
    for (auto* body : rigidBodies) {
        if (body->inverseMass > 0.0f) {
            float mass = 1.0f / body->inverseMass;
            body->ApplyForce(gravity * mass, body->globalCentroid);
        }
    }

    // 2. Integration
    for (auto* body : rigidBodies) {
        body->Integrate(dt);
    }

    // This finds potential collisions between balls
    std::vector<CollisionPair> candidates = BroadPhase::GeneratePairs(rigidBodies);

    // Phase 4: Narrow Phase (Body vs Body)
    for (auto& pair : candidates) {
        NarrowPhase::ResolveCircleCollision(pair.A, pair.B);
    }

    // It ensures balls stay in the box after hitting each other.
    ResolveBoundaries();

    std::vector<CollisionPair> pairs = BroadPhase::GeneratePairs(rigidBodies);

    // Temporary Debug
    if (!pairs.empty()) {
        // std::cout << "Broad Phase found " << pairs.size() << " potential collisions!" << std::endl;
    }
}

void PhysicsWorld::ClearForces() {
    for (auto* body : rigidBodies) {
        body->forceAccumulator.Zero();
        body->torqueAccumulator.Zero();
    }
}
