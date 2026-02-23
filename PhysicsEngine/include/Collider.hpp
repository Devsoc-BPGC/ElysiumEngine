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

    // Shape-specific data (simplified for now)
    float radius;    // For Sphere
    Vec3 halfExtents; // For Box (half-width, half-height, half-depth)

    // Constructor for a Sphere/Circle
    static Collider CreateSphere(float r, float density, Vec3 offset = Vec3(0,0,0)) {
        Collider c;
        c.type = ColliderType::Sphere;
        c.radius = r;
        c.localCentroid = offset;

        // Sphere Mass: (4/3) * PI * r^3 * density
        // Since you're doing 2D rendering, you might prefer Area-based mass:
        // PI * r^2 * density
        c.mass = 3.14159f * (r * r) * density;

        // Inertia Tensor for a sphere: (2/5) * m * r^2
        float i = (2.0f / 5.0f) * c.mass * (r * r);
        c.localInertiaTensor = Mat3::Diagonal(i, i, i);

        return c;
    }
};
