#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include "BroadPhase.hpp"
#include "RigidBody.hpp"
#include "Collider.hpp"

// Brute Force implementation for comparison
std::vector<CollisionPair> BruteForceGeneratePairs(const std::vector<RigidBody*>& bodies) {
    std::vector<CollisionPair> pairs;
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            AABB a = bodies[i]->GetAABB();
            AABB b = bodies[j]->GetAABB();
            if ((a.min.x <= b.max.x && a.max.x >= b.min.x) &&
                (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
                (a.min.z <= b.max.z && a.max.z >= b.min.z)) {
                pairs.push_back({bodies[i], bodies[j]});
            }
        }
    }
    return pairs;
}

void RunBenchmark(int numBodies) {
    std::cout << "\n--- Benchmarking BroadPhase with " << numBodies << " bodies ---" << std::endl;

    std::vector<RigidBody*> bodies;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> posDist(0.0f, 100.0f);

    for (int i = 0; i < numBodies; ++i) {
        RigidBody* b = new RigidBody();
        b->position = Vec3(posDist(rng), posDist(rng), 0);
        b->colliders.push_back(Collider::CreateSphere(0.5f, 1.0f));
        bodies.push_back(b);
    }

    // Warm up
    BruteForceGeneratePairs(bodies);
    BroadPhase::GeneratePairs(bodies);

    // 1. Benchmark Brute Force
    auto start = std::chrono::high_resolution_clock::now();
    auto pairsBrute = BruteForceGeneratePairs(bodies);
    auto end = std::chrono::high_resolution_clock::now();
    auto durationBrute = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // 2. Benchmark Spatial Hashing (optimized)
    start = std::chrono::high_resolution_clock::now();
    auto pairsOptimized = BroadPhase::GeneratePairs(bodies);
    end = std::chrono::high_resolution_clock::now();
    auto durationOptimized = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Brute Force: " << durationBrute << " us (" << pairsBrute.size() << " pairs found)" << std::endl;
    std::cout << "Spatial Hashing: " << durationOptimized << " us (" << pairsOptimized.size() << " pairs found)" << std::endl;

    if (durationOptimized < durationBrute) {
        float speedup = (float)durationBrute / durationOptimized;
        std::cout << "Speedup: " << speedup << "x" << std::endl;
    } else {
        std::cout << "Optimization is SLOWER by " << (float)durationOptimized / durationBrute << "x" << std::endl;
    }

    // Verify correctness
    std::sort(pairsBrute.begin(), pairsBrute.end(), [](const CollisionPair& a, const CollisionPair& b) {
        if (a.A != b.A) return a.A < b.A;
        return a.B < b.B;
    });
    std::sort(pairsOptimized.begin(), pairsOptimized.end(), [](const CollisionPair& a, const CollisionPair& b) {
        if (a.A != b.A) return a.A < b.A;
        return a.B < b.B;
    });

    if (pairsBrute.size() != pairsOptimized.size()) {
        std::cout << "ERROR: Pair count mismatch! Brute: " << pairsBrute.size() << ", Optimized: " << pairsOptimized.size() << std::endl;
    }

    for (auto* b : bodies) delete b;
}

int main() {
    RunBenchmark(100);
    RunBenchmark(500);
    RunBenchmark(1000);
    return 0;
}
