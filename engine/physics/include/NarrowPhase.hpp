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
     * @brief Resolves collisions between two rigid bodies by checking their colliders.
     * @param a The first rigid body.
     * @param b The second rigid body.
     */
    static void ResolveCollision(RigidBody* a, RigidBody* b) {
        for (auto& colA : a->colliders) {
            for (auto& colB : b->colliders) {
                if (colA.type == ColliderType::Sphere && colB.type == ColliderType::Sphere) {
                    ResolveSphereSphere(a, colA, b, colB);
                }
                else if (colA.type == ColliderType::Sphere && colB.type == ColliderType::Box) {
                    ResolveSphereBox(a, colA, b, colB);
                }
                else if (colA.type == ColliderType::Box && colB.type == ColliderType::Sphere) {
                    ResolveSphereBox(b, colB, a, colA);
                }
                else if (colA.type == ColliderType::Box && colB.type == ColliderType::Box) {
                    ResolveBoxBox(a, colA, b, colB);
                }
            }
        }
    }

private:
    static void ResolveBoxBox(RigidBody* a, const Collider& colA, RigidBody* b, const Collider& colB) {
        // 1. Get world positions of centroids
        Vec3 posA = a->LocalToGlobal(colA.localCentroid);
        Vec3 posB = b->LocalToGlobal(colB.localCentroid);
        Vec3 deltaPos = posB - posA;

        // 2. Axes to test: Box A's X/Y, Box B's X/Y
        // In 2D, columns 0 and 1 of the orientation matrix are X and Y world axes.
        Vec3 axes[4] = {
            a->orientation.GetColumn(0).Normalized(),
            a->orientation.GetColumn(1).Normalized(),
            b->orientation.GetColumn(0).Normalized(),
            b->orientation.GetColumn(1).Normalized()
        };

        float minOverlap = 1e10f;
        Vec3 mtvNormal;

        for (int i = 0; i < 4; ++i) {
            Vec3 axis = axes[i];

            // Project Box A
            float projectionA = 
                std::abs(axes[0].Dot(axis)) * colA.halfExtents.x +
                std::abs(axes[1].Dot(axis)) * colA.halfExtents.y;

            // Project Box B
            float projectionB = 
                std::abs(axes[2].Dot(axis)) * colB.halfExtents.x +
                std::abs(axes[3].Dot(axis)) * colB.halfExtents.y;

            // Project distance between centers
            float distance = std::abs(deltaPos.Dot(axis));

            float overlap = projectionA + projectionB - distance;

            if (overlap <= 0) return; // Separating axis found

            if (overlap < minOverlap) {
                minOverlap = overlap;
                mtvNormal = axis;
            }
        }

        // Ensure normal points from A to B
        if (deltaPos.Dot(mtvNormal) < 0) {
            mtvNormal = mtvNormal * -1.0f;
        }

        ApplyImpulse(a, b, mtvNormal, minOverlap);
    }
    static void ResolveSphereSphere(RigidBody* a, const Collider& colA, RigidBody* b, const Collider& colB) {
        Vec3 posA = a->LocalToGlobal(colA.localCentroid);
        Vec3 posB = b->LocalToGlobal(colB.localCentroid);
        float radiusA = colA.radius;
        float radiusB = colB.radius;

        Vec3 normal = posB - posA; // From A to B
        float distSq = normal.MagnitudeSquared();
        float radiusSum = radiusA + radiusB;

        if (distSq >= radiusSum * radiusSum || distSq == 0) return;

        float distance = std::sqrt(distSq);
        normal = normal / distance;

        ApplyImpulse(a, b, normal, radiusSum - distance);
    }

    static void ResolveSphereBox(RigidBody* sphereBody, const Collider& sphereCol, RigidBody* boxBody, const Collider& boxCol) {
        // 1. Get sphere position in box's local space
        Vec3 sphereWorldPos = sphereBody->LocalToGlobal(sphereCol.localCentroid);
        Vec3 sphereLocalPos = boxBody->GlobalToLocal(sphereWorldPos);

        // 2. Find the closest point on the box to the sphere center
        Vec3 closestPointLocal = sphereLocalPos;
        Vec3 halfExtents = boxCol.halfExtents;

        if (closestPointLocal.x > halfExtents.x) closestPointLocal.x = halfExtents.x;
        else if (closestPointLocal.x < -halfExtents.x) closestPointLocal.x = -halfExtents.x;

        if (closestPointLocal.y > halfExtents.y) closestPointLocal.y = halfExtents.y;
        else if (closestPointLocal.y < -halfExtents.y) closestPointLocal.y = -halfExtents.y;

        if (closestPointLocal.z > halfExtents.z) closestPointLocal.z = halfExtents.z;
        else if (closestPointLocal.z < -halfExtents.z) closestPointLocal.z = -halfExtents.z;

        // 3. Check if the closest point is within radius distance
        Vec3 vecFromClosestToSphereLocal = sphereLocalPos - closestPointLocal;
        float distSq = vecFromClosestToSphereLocal.MagnitudeSquared();
        
        if (distSq >= sphereCol.radius * sphereCol.radius || distSq == 0) return;

        float distance = std::sqrt(distSq);
        
        // normalFromAtoB where A=Sphere, B=Box
        // normalLocal = closestPointLocal - sphereLocalPos
        Vec3 normalLocal = closestPointLocal - sphereLocalPos;
        Vec3 normalWorld = boxBody->LocalToGlobalVec(normalLocal / distance);

        ApplyImpulse(sphereBody, boxBody, normalWorld, sphereCol.radius - distance);
    }

    static void ApplyImpulse(RigidBody* a, RigidBody* b, const Vec3& normalFromAtoB, float overlap) {
        // 1. Position Correction (to resolve overlap)
        float totalInvMass = a->inverseMass + b->inverseMass;
        if (totalInvMass > 0) {
            Vec3 separation = normalFromAtoB * (overlap / totalInvMass);
            a->position -= separation * a->inverseMass;
            b->position += separation * b->inverseMass;

            a->UpdateGlobalCentroidFromPosition();
            b->UpdateGlobalCentroidFromPosition();
        }

        // 2. Impulse Resolution (to change velocity)
        Vec3 relativeVel = b->linearVelocity - a->linearVelocity;
        float velAlongNormal = relativeVel.Dot(normalFromAtoB);

        // Do not resolve if velocities are separating (relative velocity from A to B is positive)
        if (velAlongNormal > 0) return;

        float restitution = 0.8f;
        float j = -(1.0f + restitution) * velAlongNormal;
        j /= totalInvMass;

        Vec3 impulse = normalFromAtoB * j;
        a->linearVelocity -= impulse * a->inverseMass;
        b->linearVelocity += impulse * b->inverseMass;
    }
};

#endif
