#ifndef NARROW_PHASE_HPP
#define NARROW_PHASE_HPP

#include "RigidBody.hpp"
#include <cmath>

/**
 * @class NarrowPhase
 * @brief A static utility class used to resolve collisions between rigid bodies.
 */
class NarrowPhase {
public:
    /**
     * @brief Resolves collisions between two rigid bodies by checking all their circle colliders.
     * @param a The first rigid body.
     * @param b The second rigid body.
     */
    static void ResolveCircleCollision(RigidBody* a, RigidBody* b) {
        for (auto& colA : a->colliders) {
            for (auto& colB : b->colliders) {
                if (colA.type != ColliderType::Sphere || colB.type != ColliderType::Sphere)
                    continue;

                Vec3 posA = a->LocalToGlobal(colA.localCentroid);
                Vec3 posB = b->LocalToGlobal(colB.localCentroid);
                float radiusA = colA.radius;
                float radiusB = colB.radius;

                Vec3 normal = posB - posA;
                float distSq = normal.MagnitudeSquared();
                float radiusSum = radiusA + radiusB;

                if (distSq >= radiusSum * radiusSum || distSq == 0) continue;

                float distance = std::sqrt(distSq);
                normal = normal / distance;

                // 1. Position Correction (to resolve overlap)
                float overlap = radiusSum - distance;
                float totalInvMass = a->inverseMass + b->inverseMass;
                if (totalInvMass > 0) {
                    Vec3 separation = normal * (overlap / totalInvMass);
                    a->position -= separation * a->inverseMass;
                    b->position += separation * b->inverseMass;

                    a->UpdateGlobalCentroidFromPosition();
                    b->UpdateGlobalCentroidFromPosition();
                }

                // 2. Impulse Resolution (to change velocity)
                Vec3 relativeVel = b->linearVelocity - a->linearVelocity;
                float velAlongNormal = relativeVel.Dot(normal);

                // Do not resolve if velocities are separating
                if (velAlongNormal > 0) continue;

                float restitution = 0.8f;
                float j = -(1.0f + restitution) * velAlongNormal;
                j /= totalInvMass;

                Vec3 impulse = normal * j;
                a->linearVelocity -= impulse * a->inverseMass;
                b->linearVelocity += impulse * b->inverseMass;
            }
        }
    }
};

#endif
