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
        float i = 0.5f * c.mass * (r * r);
        c.localInertiaTensor = Mat3::Diagonal(i, i, i);

        return c;
    }

    /**
     * @brief Constructor for a Box.
     * @param extents Half-widths in X, Y, Z.
     * @param density Mass per unit volume.
     * @param offset Local offset from the body's origin.
     */
    static Collider CreateBox(Vec3 extents, float density, Vec3 offset = Vec3(0,0,0)) {
        Collider c;
        c.type = ColliderType::Box;
        c.halfExtents = extents;
        c.localCentroid = offset;

        // 2D Box Area: (2*ex) * (2*ey)
        c.mass = (2.0f * extents.x) * (2.0f * extents.y) * density;

        // 2D Inertia Tensor for a rectangle: (1/12) * m * (w^2 + h^2)
        float w = 2.0f * extents.x;
        float h = 2.0f * extents.y;
        float i = (1.0f / 12.0f) * c.mass * (w * w + h * h);
        c.localInertiaTensor = Mat3::Diagonal(i, i, i);

        // For AABB calculations, we still use a radius-like value or handle it specifically.
        // For now, let's use the distance to the furthest corner as a "bounding radius".
        c.radius = extents.Magnitude();

        return c;
    }
};

#endif
