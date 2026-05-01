#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include "NarrowPhase.hpp"
#include "RigidBody.hpp"
#include "Collider.hpp"

// Simple performance test for collision resolution algorithms
void RunPerformanceTest(int iterations, const std::string& type) {
    std::cout << "Running performance test for " << type << " (" << iterations << " iterations)..." << std::endl;

    std::vector<RigidBody> bodiesA(iterations);
    std::vector<RigidBody> bodiesB(iterations);
    std::vector<Collider> collidersA(iterations);
    std::vector<Collider> collidersB(iterations);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28f);

    for (int i = 0; i < iterations; ++i) {
        bodiesA[i].position = Vec3(dist(rng), dist(rng), 0);
        bodiesB[i].position = Vec3(dist(rng), dist(rng), 0);
        
        float angleA = angleDist(rng);
        float angleB = angleDist(rng);
        bodiesA[i].orientation = Mat3::RotationZ(angleA);
        bodiesB[i].orientation = Mat3::RotationZ(angleB);
        
        bodiesA[i].inverseMass = 1.0f;
        bodiesB[i].inverseMass = 1.0f;

        if (type == "Sphere-Sphere") {
            collidersA[i] = Collider::CreateSphere(1.0f, 1.0f);
            collidersB[i] = Collider::CreateSphere(1.0f, 1.0f);
        } else if (type == "Sphere-Box") {
            collidersA[i] = Collider::CreateSphere(1.0f, 1.0f);
            collidersB[i] = Collider::CreateBox(Vec3(1.0f, 1.0f, 0.5f), 1.0f);
        } else if (type == "Box-Box") {
            collidersA[i] = Collider::CreateBox(Vec3(1.0f, 1.0f, 0.5f), 1.0f);
            collidersB[i] = Collider::CreateBox(Vec3(1.0f, 1.0f, 0.5f), 1.0f);
        }
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        NarrowPhase::ResolveCollision(&bodiesA[i], &bodiesB[i]);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << type << " took: " << duration << " us (" << (float)duration / iterations << " us/op)" << std::endl;
}

int main() {
    int iterations = 100000;

    RunPerformanceTest(iterations, "Sphere-Sphere");
    RunPerformanceTest(iterations, "Sphere-Box");
    RunPerformanceTest(iterations, "Box-Box");

    return 0;
}
