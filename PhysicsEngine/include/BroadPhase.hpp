/**
 * @file BroadPhase.hpp
 * @brief Provides efficient spatial pruning to find potential collision pairs.
 */

#ifndef BROADPHASE_HPP
#define BROADPHASE_HPP

#include <vector>
#include "RigidBody.hpp"
#include "CollisionPair.hpp"

/**
 * @class BroadPhase
 * @brief A static utility class used to detect potential overlap between rigid bodies using AABBs.
 * * The Broad Phase is the first stage of the collision pipeline. It uses simple Axis-Aligned Bounding
 * Boxes (AABBs) to perform fast intersection tests, avoiding expensive narrow-phase geometric
 * checks for objects that are clearly separated.
 */
class BroadPhase {
public:
    /**
     * @brief Identifies all pairs of rigid bodies whose AABBs intersect.
     * * This implementation uses a "Brute Force" or "All-Pairs" approach, which has a
     * time complexity of $O(n^2)$.
     * * @param bodies A list of pointers to all rigid bodies in the simulation.
     * @return A vector of CollisionPair objects containing pointers to bodies that might be colliding.
     */
    static std::vector<CollisionPair> GeneratePairs(const std::vector<RigidBody*>& bodies) {
        std::vector<CollisionPair> pairs;

        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                AABB a = bodies[i]->GetAABB();
                AABB b = bodies[j]->GetAABB();

                if (Intersect(a, b)) {
                    pairs.push_back({bodies[i], bodies[j]});
                }
            }
        }
        return pairs;
    }

private:
    /**
     * @brief Performs a standard AABB vs AABB overlap test.
     * * The test checks for overlap on all three principal axes (X, Y, and Z).
     * If there is an overlap on every axis, the boxes intersect.
     * * @param a The first Bounding Box.
     * @param b The second Bounding Box.
     * @return true if the boxes overlap, false otherwise.
     */
    static bool Intersect(const AABB& a, const AABB& b) {
        return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
               (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
               (a.min.z <= b.max.z && a.max.z >= b.min.z);
    }
};

#endif
