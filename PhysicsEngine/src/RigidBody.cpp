/**
 * @file RigidBody.cpp
 * @brief Implementation of RigidBody physics dynamics, coordinate transformations, and integration.
 */

#include "RigidBody.hpp"
#include "CoreMath.hpp"

/**
 * @brief Computes the world-space center of mass based on the body's position and orientation.
 * * Formula: $globalCentroid = R \cdot localCentroid + position$
 */
void RigidBody::UpdateGlobalCentroidFromPosition(void) {
    globalCentroid = orientation * localCentroid + position;
}

/**
 * @brief Updates the body's origin position based on the current global center of mass.
 * * This ensures that the geometric "anchor" stays in sync with the physics center of mass.
 */
void RigidBody::UpdatePositionFromGlobalCentroid(void) {
    position = orientation * (-localCentroid) + globalCentroid;
}

/**
 * @brief Adds a collider to the body and recalculates mass properties.
 * * This method updates the total mass, the local center of mass (centroid), and
 * uses the **Parallel Axis Theorem** to compute the new local inertia tensor.
 * * @param collider The collider to be added to the rigid body.
 */
void RigidBody::AddColliders(const Collider &collider) {
    colliders.push_back(collider);

    localCentroid.Zero();
    mass = 0.0f;

    // Calculate total mass and weighted centroid
    for(Collider &collider : colliders) {
        mass += collider.mass;
        localCentroid += collider.mass * collider.localCentroid;
    }

    inverseMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
    localCentroid *= inverseMass;

    Mat3 localInertiaTensor;
    localInertiaTensor.Zero();

    // Sum inertia tensors using the Parallel Axis Theorem
    for (Collider &collider : colliders) {
        const Vec3 r = localCentroid - collider.localCentroid;
        const float rDotR = r.Dot(r);
        const Mat3 rOutR = r.outerProduct(r);

        // I_total = sum( I_local + mass * ( (r·r)I - r⊗r ) )
        localInertiaTensor += collider.localInertiaTensor + collider.mass * (rDotR * Mat3::Identity() - rOutR);
    }

    localInverseInertiaTensor = localInertiaTensor.Inverted();
}

/**
 * @brief Transforms a point from local coordinates to world coordinates.
 * @param p Point in local space.
 * @return Point in world space.
 */
const Vec3 RigidBody::LocalToGlobal(const Vec3 &p) const {
    return orientation * p + position;
}

/**
 * @brief Transforms a point from world coordinates to local coordinates.
 * @param p Point in world space.
 * @return Point in local space.
 */
const Vec3 RigidBody::GlobalToLocal(const Vec3 &p) const {
    return inverseOrientation * (p - position);
}

/**
 * @brief Transforms a direction vector from local to world space.
 * @note This ignores translation (position).
 * @param v Vector in local space.
 */
const Vec3 RigidBody::LocalToGlobalVec(const Vec3 &v) const {
    return orientation * v;
}

/**
 * @brief Transforms a direction vector from world to local space.
 * @note This ignores translation (position).
 * @param v Vector in world space.
 */
const Vec3 RigidBody::GlobalToLocalVec(const Vec3 &v) const {
    return inverseOrientation * v;
}

/**
 * @brief Applies a force at a specific world-space location.
 * * Generates linear acceleration and torque based on the offset from the center of mass.
 * @param f The force vector.
 * @param at The world-space position where the force hits.
 */
void RigidBody::ApplyForce(const Vec3 &f, const Vec3 &at) {
    forceAccumulator += f;
    torqueAccumulator += (at - globalCentroid).Cross(f);
}

/**
 * @brief Normalizes the orientation matrix to prevent floating-point drift.
 * * Converts the matrix to a Quaternion, normalizes it, and converts back to a Matrix.
 * Also updates the inverse (transposed) orientation.
 */
void RigidBody::UpdateOrientation(void) {
    Quat q = orientation.ToQuat();
    q.Normalize();
    orientation = q.ToMatrix();

    inverseOrientation = orientation.Transposed();
}

/**
 * @brief Advances the physics state by a time step.
 * * Updates linear/angular velocities, applies translation to the centroid,
 * performs rotational integration, and synchronizes the body position.
 * * @param dt Time step in seconds.
 */
void RigidBody::Integrate(float dt) {
    if (inverseMass <= 0.0f) return;

    // Linear Physics
    Vec3 linearAcceleration = forceAccumulator * inverseMass;
    linearVelocity += linearAcceleration * dt;

    // Angular Physics
    Vec3 angularAcceleration = inverseInertiaTensorWorld * torqueAccumulator;
    angularVelocity += angularAcceleration * dt;

    // Clear accumulators for next frame
    forceAccumulator.Zero();
    torqueAccumulator.Zero();

    // Apply linear movement
    globalCentroid += linearVelocity * dt;

    // Apply rotational movement
    if (angularVelocity.MagnitudeSquared() > EPSILON) {
        Vec3 axis = angularVelocity.Normalized();
        float angle = angularVelocity.Magnitude() * dt;
        orientation = Mat3::RotationMatrix(axis, angle) * orientation;
    }

    UpdateOrientation();
    UpdatePositionFromGlobalCentroid();
}
