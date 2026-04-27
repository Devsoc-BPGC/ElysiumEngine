#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include "CoreMath.hpp"

enum class ColliderType {
    Sphere,
    Box
};

class Collider {
public:
    ColliderType type;

    // Local transform relative to the RigidBody's 'position'
    Vec3 localCentroid;

    // Physical properties
    float mass;
    Mat3 localInertiaTensor;

    // Shape-specific data
    float radius;    // For Sphere/Circle
    Vec3 halfExtents; // For Box

    /**
     * @brief Constructor for a Circle (Sphere in 3D math terms).
     * @param r Radius of the circle.
     * @param density Mass per unit area.
     * @param offset Local offset from the body's origin.
     */
    static Collider CreateSphere(float r, float density, Vec3 offset = Vec3(0,0,0)) {
        Collider c;
        c.type = ColliderType::Sphere;
        c.radius = r;
        c.localCentroid = offset;

        // 2D Circle Area-based mass: PI * r^2 * density
        c.mass = PI * (r * r) * density;

        // 2D Inertia Tensor for a disk (around Z axis): (1/2) * m * r^2
        // We set the diagonal for X and Y to something reasonable or the same for 2D stability,
        // but the Z component is what matters for 2D rotation.
        float i = 0.5f * c.mass * (r * r);
        c.localInertiaTensor = Mat3::Diagonal(i, i, i);

        return c;
    }
};

#endif
