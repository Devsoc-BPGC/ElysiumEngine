/**
 * @file BroadPhase.hpp
 * @brief Provides efficient spatial pruning to find potential collision pairs.
 */

#ifndef BROADPHASE_HPP
#define BROADPHASE_HPP

#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>
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
    static constexpr float CELL_SIZE = 4.0f;

    /**
     * @brief Identifies all pairs of rigid bodies whose AABBs intersect.
     * * This implementation uses a Spatial Hashing approach for efficiency.
     * * @param bodies A list of pointers to all rigid bodies in the simulation.
     * @return A vector of CollisionPair objects containing pointers to bodies that might be colliding.
     */
    static std::vector<CollisionPair> GeneratePairs(const std::vector<RigidBody*>& bodies) {
        if (bodies.size() < 2) return {};

        std::unordered_map<size_t, std::vector<RigidBody*>> grid;

        auto GetHash = [](int x, int y) -> size_t {
            return (static_cast<size_t>(x) * 73856093) ^ (static_cast<size_t>(y) * 19349663);
        };

        // 1. Insert bodies into the grid
        for (auto* body : bodies) {
            AABB aabb = body->GetAABB();
            int minX = static_cast<int>(std::floor(aabb.min.x / CELL_SIZE));
            int minY = static_cast<int>(std::floor(aabb.min.y / CELL_SIZE));
            int maxX = static_cast<int>(std::floor(aabb.max.x / CELL_SIZE));
            int maxY = static_cast<int>(std::floor(aabb.max.y / CELL_SIZE));

            for (int x = minX; x <= maxX; ++x) {
                for (int y = minY; y <= maxY; ++y) {
                    grid[GetHash(x, y)].push_back(body);
                }
            }
        }

        // 2. Collect pairs from grid cells
        std::vector<CollisionPair> pairs;
        for (auto& [hash, cellBodies] : grid) {
            if (cellBodies.size() < 2) continue;

            for (size_t i = 0; i < cellBodies.size(); ++i) {
                for (size_t j = i + 1; j < cellBodies.size(); ++j) {
                    RigidBody* a = cellBodies[i];
                    RigidBody* b = cellBodies[j];

                    // Ensure A < B for consistent duplicate removal
                    if (a > b) std::swap(a, b);
                    pairs.push_back({a, b});
                }
            }
        }

        // 3. Remove duplicate pairs and perform actual AABB intersection check
        std::sort(pairs.begin(), pairs.end(), [](const CollisionPair& lhs, const CollisionPair& rhs) {
            if (lhs.A != rhs.A) return lhs.A < rhs.A;
            return lhs.B < rhs.B;
        });
        pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());

        // Final Narrow-AABB check
        std::vector<CollisionPair> result;
        result.reserve(pairs.size());
        for (const auto& pair : pairs) {
            if (Intersect(pair.A->GetAABB(), pair.B->GetAABB())) {
                result.push_back(pair);
            }
        }

        return result;
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
