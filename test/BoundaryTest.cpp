#include "PhysicsWorld.hpp"
#include <cassert>
#include <iostream>

int main() {
    PhysicsWorld world;

    // Check defaults
    assert(world.boundaryCenter.x == 8.0f);
    assert(world.boundaryCenter.y == 6.0f);
    assert(world.boundaryHalfExtents.x == 5.0f);
    assert(world.boundaryHalfExtents.y == 5.0f);
    assert(world.boundaryRotation == 0.0f);

    // Test SetBoundaries
    Vec3 newCenter(10.0f, 10.0f, 0.0f);
    Vec3 newHalfExtents(20.0f, 20.0f, 0.0f);
    float newRotation = 0.5f;

    world.SetBoundaries(newCenter, newHalfExtents, newRotation);

    assert(world.boundaryCenter.x == 10.0f);
    assert(world.boundaryCenter.y == 10.0f);
    assert(world.boundaryHalfExtents.x == 20.0f);
    assert(world.boundaryHalfExtents.y == 20.0f);
    assert(world.boundaryRotation == 0.5f);

    std::cout << "Boundary configuration test passed!" << std::endl;
    return 0;
}
