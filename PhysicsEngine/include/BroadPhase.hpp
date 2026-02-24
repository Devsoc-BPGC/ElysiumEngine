#ifndef BROADPHASE_HPP
#define BROADPHASE_HPP

#include <vector>
#include "RigidBody.hpp"
#include "CollisionPair.hpp"
// #include <iostream>

class BroadPhase {
public:
static std::vector<CollisionPair> GeneratePairs(const std::vector<RigidBody*>& bodies) {
    std::vector<CollisionPair> pairs;
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            AABB a = bodies[i]->GetAABB();
            AABB b = bodies[j]->GetAABB();

            if (Intersect(a, b)) {
                pairs.push_back({bodies[i], bodies[j]});
            } else {
                // std::cout << "Body " << i << " MinX: " << a.min.x << " MaxX: " << a.max.x << std::endl;
                // std::cout << "Body " << j << " MinX: " << b.min.x << " MaxX: " << b.max.x << std::endl;
            }
        }
    }
    return pairs;
}

private:
    static bool Intersect(const AABB& a, const AABB& b) {
        // Standard AABB vs AABB overlap test
        return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
               (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
               (a.min.z <= b.max.z && a.max.z >= b.min.z);
    }
};

#endif
