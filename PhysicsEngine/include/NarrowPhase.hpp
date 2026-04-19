#ifndef NARROW_PHASE_HPP
#define NARROW_PHASE_HPP

#include "RigidBody.hpp"
#include <cmath>

/**
 * @class NarrowPhase
 * @brief A static utility class used to resolve collisions between rigid bodies.
 * * The Narrow Phase is the second stage of the collision pipeline. It uses geometric
 * checks (e.g., circle-circle, AABB-AABB) to determine if two bodies are colliding,
 * and computes the impulse needed to resolve the collision.
 */
class NarrowPhase {
public:
    /**
     * @brief Resolves a collision between two rigid bodies using a circle-circle collision test.
     * * This method computes the impulse needed to push the colliding bodies apart,
     * taking into account their mass, inertia, and position.
     * * @param a The first rigid body.
     * @param b The second rigid body.
     */
    static void ResolveCircleCollision(RigidBody* a, RigidBody* b) {
        Vec3 posA = a->LocalToGlobal(a->colliders[0].localCentroid);
        Vec3 posB = b->LocalToGlobal(b->colliders[0].localCentroid);
        float radiusA = a->colliders[0].radius;
        float radiusB = b->colliders[0].radius;

        Vec3 normal = posB - posA;
        float distSq = normal.MagnitudeSquared();
        float radiusSum = radiusA + radiusB;

        if (distSq >= radiusSum * radiusSum || distSq == 0) return;

        float distance = std::sqrt(distSq);
        normal = normal / distance;

        float overlap = radiusSum - distance;
        float totalInvMass = a->inverseMass + b->inverseMass;
        if (totalInvMass > 0) {
            Vec3 separation = normal * (overlap / totalInvMass);
            a->position -= separation * a->inverseMass;
            b->position += separation * b->inverseMass;

            a->UpdateGlobalCentroidFromPosition();
            b->UpdateGlobalCentroidFromPosition();
        }

        Vec3 relativeVel = b->linearVelocity - a->linearVelocity;
        float velAlongNormal = relativeVel.Dot(normal);

        if (velAlongNormal > 0) return;

        float restitution = 0.8f;
        float j = -(1.0f + restitution) * velAlongNormal;
        j /= totalInvMass;

        Vec3 impulse = normal * j;
        a->linearVelocity -= impulse * a->inverseMass;
        b->linearVelocity += impulse * b->inverseMass;
    }
};

#endif
