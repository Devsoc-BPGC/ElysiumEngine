#ifndef RIGIDBODY_HPP
#define RIGIDBODY_HPP

#include <vector>
#include "CoreMath.hpp"
#include "Collider.hpp"

typedef std::vector<Collider> ColliderList;

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
    void AddColliders(Collider &collider);
    void Integrate(float dt);

    // Coordinate Transformations
    const Vec3 LocalToGlobal(const Vec3 &p) const;
    const Vec3 GlobalToLocal(const Vec3 &p) const;
    const Vec3 LocalToGlobalVec(const Vec3 &v) const;
    const Vec3 GlobalToLocalVec(const Vec3 &v) const;

    void ApplyForce(const Vec3 &f, const Vec3 &at);
};

#endif
