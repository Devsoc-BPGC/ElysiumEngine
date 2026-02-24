#ifndef RIGIDBODY_HPP
#define RIGIDBODY_HPP

#include <vector>
#include "CoreMath.hpp"
#include "Collider.hpp"

typedef std::vector<Collider> ColliderList;

struct AABB {
    Vec3 min;
    Vec3 max;
};

struct RigidBody {
    float mass;
    float inverseMass;


    Mat3 orientation;
    Mat3 inverseOrientation;

    Mat3 localInverseInertiaTensor;
    Mat3 inverseInertiaTensorWorld;

    Vec3 globalCentroid;
    Vec3 localCentroid;

    Vec3 position;
    Vec3 linearVelocity;
    Vec3 angularVelocity;

    Vec3 forceAccumulator;
    Vec3 torqueAccumulator;

    ColliderList colliders;

    // Helpers to keep position and centroid in sync
    void UpdateGlobalCentroidFromPosition(void);
    void UpdatePositionFromGlobalCentroid(void);
    void UpdateOrientation(void);

    // Matches your RigidBody.cpp implementation
    void AddColliders(const Collider &collider);
    void Integrate(float dt);

    // Coordinate Transformations
    const Vec3 LocalToGlobal(const Vec3 &p) const;
    const Vec3 GlobalToLocal(const Vec3 &p) const;
    const Vec3 LocalToGlobalVec(const Vec3 &v) const;
    const Vec3 GlobalToLocalVec(const Vec3 &v) const;

    void ApplyForce(const Vec3 &f, const Vec3 &at);

    AABB GetAABB() const {
        AABB box;
        if (colliders.empty()) {
            return { position, position };
        }

        Vec3 firstPos = LocalToGlobal(colliders[0].localCentroid);
        box.min = firstPos - Vec3(colliders[0].radius, colliders[0].radius, 0);
        box.max = firstPos + Vec3(colliders[0].radius, colliders[0].radius, 0);

        for (size_t i = 1; i < colliders.size(); ++i) {
            Vec3 worldPos = LocalToGlobal(colliders[i].localCentroid);
            float r = colliders[i].radius;

            if (worldPos.x - r < box.min.x) box.min.x = worldPos.x - r;
            if (worldPos.y - r < box.min.y) box.min.y = worldPos.y - r;
            if (worldPos.x + r > box.max.x) box.max.x = worldPos.x + r;
            if (worldPos.y + r > box.max.y) box.max.y = worldPos.y + r;
        }
        return box;
    }
};

#endif
