#include "../include/PhysicsWorld.hpp"

void PhysicsWorld::Step(float dt) {
    // 1. Apply Global Forces (like Gravity)
    for (auto* body : rigidBodies) {
        if (body->inverseMass > 0.0f) {
            float mass = 1.0f / body->inverseMass;
            body->AddForce(gravity * mass);
        }
    }

    // 2. INTEGRATION PHASE
    for (auto* body : rigidBodies) {
        body->Integrate(dt); 
    }

    // 3. Collision Detection
    // 4. Constraint Solver
}
