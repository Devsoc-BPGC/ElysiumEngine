#include "PhysicsWorld.hpp"
#include "NarrowPhase.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

int main() {
    RigidBody bodyA;
    bodyA.mass = 1.0f;
    bodyA.inverseMass = 1.0f;
    bodyA.position = Vec3(0.0f, 0.0f, 0.0f);
    bodyA.linearVelocity = Vec3(10.0f, 5.0f, 0.0f);
    bodyA.friction = 0.5f;
    bodyA.AddColliders(Collider::CreateSphere(1.0f, 1.0f));
    bodyA.UpdateGlobalCentroidFromPosition();

    RigidBody bodyB;
    bodyB.mass = 0.0f; // Static
    bodyB.inverseMass = 0.0f;
    bodyB.position = Vec3(1.9f, 0.0f, 0.0f); // Slight overlap to trigger collision
    bodyB.linearVelocity = Vec3(0.0f, 0.0f, 0.0f);
    bodyB.friction = 0.5f;
    bodyB.AddColliders(Collider::CreateSphere(1.0f, 1.0f));
    bodyB.UpdateGlobalCentroidFromPosition();

    std::cout << "Before collision: bodyA.linearVelocity = (" 
              << bodyA.linearVelocity.x << ", " << bodyA.linearVelocity.y << ")" << std::endl;

    // Resolve collision
    NarrowPhase::ResolveCollision(&bodyA, &bodyB);

    std::cout << "After collision: bodyA.linearVelocity = (" 
              << bodyA.linearVelocity.x << ", " << bodyA.linearVelocity.y << ")" << std::endl;

    // Without friction, Y velocity would be exactly 5.0f.
    // With friction, it should be less than 5.0f because it's opposing the tangent motion.
    assert(bodyA.linearVelocity.y < 5.0f);
    
    // Check if it actually reduced
    if (bodyA.linearVelocity.y < 5.0f) {
        std::cout << "Friction test passed! Tangent velocity reduced from 5.0 to " << bodyA.linearVelocity.y << std::endl;
    } else {
        std::cout << "Friction test failed! Tangent velocity did not reduce." << std::endl;
        return 1;
    }

    return 0;
}
