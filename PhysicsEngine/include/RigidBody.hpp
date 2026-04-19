/**
 * @file RigidBody.hpp
 * @brief Defines the RigidBody structure and associated Axis-Aligned Bounding Box (AABB) for physics simulation.
 */

#ifndef RIGIDBODY_HPP
#define RIGIDBODY_HPP

#include <vector>
#include "CoreMath.hpp"
#include "Collider.hpp"

/** * @brief A collection of colliders attached to a single rigid body.
 */
typedef std::vector<Collider> ColliderList;

/**
 * @struct AABB
 * @brief Simple Axis-Aligned Bounding Box used for broad-phase collision detection.
 */
struct AABB {
    Vec3 min; /**< The minimum point of the box (bottom-left-back). */
    Vec3 max; /**< The maximum point of the box (top-right-front). */
};

/**
 * @struct RigidBody
 * @brief Represents a physical object with mass, velocity, and rotational properties.
 */
struct RigidBody {
    float mass;              /**< Total mass of the body. */
    float inverseMass;       /**< Precomputed 1/mass (0 for static objects). */

    Mat3 orientation;        /**< Current rotation matrix in world space. */
    Mat3 inverseOrientation; /**< Cached transpose/inverse of the orientation matrix. */

    Mat3 localInverseInertiaTensor;  /**< Inverse inertia tensor in model space. */
    Mat3 inverseInertiaTensorWorld; /**< Inverse inertia tensor transformed into world space. */

    Vec3 globalCentroid;     /**< The center of mass in world coordinates. */
    Vec3 localCentroid;      /**< The center of mass relative to the body's local origin. */

    Vec3 position;           /**< The world position of the body's origin. */
    Vec3 linearVelocity;     /**< The rate of change of position. */
    Vec3 angularVelocity;    /**< The rate of change of orientation (radians/sec). */

    Vec3 forceAccumulator;   /**< Sum of all linear forces to be applied in the next step. */
    Vec3 torqueAccumulator;  /**< Sum of all torques to be applied in the next step. */

    ColliderList colliders;  /**< List of collision geometries attached to this body. */

    /** @name Transformation Synchronizers */
    ///@{
    /** @brief Updates the global centroid based on current position and orientation. */
    void UpdateGlobalCentroidFromPosition(void);
    /** @brief Updates the position based on the global centroid. */
    void UpdatePositionFromGlobalCentroid(void);
    /** @brief Updates the rotation matrices (orthonormalization and inverse calculation). */
    void UpdateOrientation(void);
    ///@}

    /**
     * @brief Adds a collider to the rigid body's internal list.
     * @param collider The collider geometry to add.
     */
    void AddColliders(const Collider &collider);

    /**
     * @brief Performs numerical integration to update velocity and position.
     * @param dt The time step in seconds.
     */
    void Integrate(float dt);

    /** @name Coordinate Transformations */
    ///@{
    /** @brief Transforms a point from local space to world space. */
    const Vec3 LocalToGlobal(const Vec3 &p) const;
    /** @brief Transforms a point from world space to local space. */
    const Vec3 GlobalToLocal(const Vec3 &p) const;
    /** @brief Transforms a direction vector from local space to world space (no translation). */
    const Vec3 LocalToGlobalVec(const Vec3 &v) const;
    /** @brief Transforms a direction vector from world space to local space (no translation). */
    const Vec3 GlobalToLocalVec(const Vec3 &v) const;
    ///@}

    /**
     * @brief Applies a force at a specific world-space point, generating torque.
     * @param f The force vector to apply.
     * @param at The world-space position where the force is applied.
     */
    void ApplyForce(const Vec3 &f, const Vec3 &at);

    /**
     * @brief Calculates the total AABB encompassing all attached colliders.
     * @return An AABB that tightly fits the body's current world-space state.
     */
    AABB GetAABB() const {
        AABB box;
        if (colliders.empty()) {
            return { position, position };
        }

        // Initialize with the first collider
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
