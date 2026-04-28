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
            }
        }
    }

private:
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
