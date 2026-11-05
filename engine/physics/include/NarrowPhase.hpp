#ifndef NARROW_PHASE_HPP
#define NARROW_PHASE_HPP

#include "RigidBody.hpp"
#include "SAT.hpp"
#include <cmath>


/**
 * @class NarrowPhase
 * @brief A static utility class used to resolve collisions between rigid
 * bodies.
 */
class NarrowPhase {
public:
  /**
   * @brief Resolves collisions between two rigid bodies by checking their
   * colliders.
   * @param a The first rigid body.
   * @param b The second rigid body.
   */
  static bool IsConvexShape(ColliderType type) {
    return type == ColliderType::Box || type == ColliderType::Polygon;
  }

  static void ResolveCollision(RigidBody *a, RigidBody *b) {
    for (auto &colA : a->colliders) {
      for (auto &colB : b->colliders) {
        bool isPolyA = IsConvexShape(colA.type);
        bool isPolyB = IsConvexShape(colB.type);

        if (colA.type == ColliderType::Sphere && colB.type == ColliderType::Sphere) {
          ResolveSphereSphere(a, colA, b, colB);
        } else if (colA.type == ColliderType::Sphere && isPolyB) {
          ResolveCirclePolygon(a, colA, b, colB);
        } else if (isPolyA && colB.type == ColliderType::Sphere) {
          ResolveCirclePolygon(b, colB, a, colA);
        } else if (isPolyA && isPolyB) {
          ResolvePolygonPolygon(a, colA, b, colB);
        }
      }
    }
  }



private:

  static void ResolveCirclePolygon(RigidBody *circleBody, const Collider &colCircle, RigidBody *polyBody, const Collider &colPoly) {
    Vec3 circleCenter = circleBody->LocalToGlobal(colCircle.localCentroid);
    Vec3 polyCenter = polyBody->LocalToGlobal(colPoly.localCentroid);
    // Cheap bounding-radius early-out: avoids allocating world vertices when shapes are far apart
    float radiusSum = colCircle.radius + colPoly.radius;
    float distSq = (circleCenter - polyCenter).MagnitudeSquared();
    if (distSq >= radiusSum * radiusSum || distSq == 0.0f) {
      return;
    }
    std::vector<Vec3> polyVerts = colPoly.GetWorldVertices(*polyBody);
    Elysium::SATResult sat = Elysium::SAT::TestCirclePolygon(circleCenter, colCircle.radius, polyVerts);
    if (sat.collided) {
      ApplyImpulse(circleBody, polyBody, sat.normal, sat.depth);
    }
  }

  static void ResolvePolygonPolygon(RigidBody *a, const Collider &colA, RigidBody *b, const Collider &colB) {
    Vec3 centerA = a->LocalToGlobal(colA.localCentroid);
    Vec3 centerB = b->LocalToGlobal(colB.localCentroid);
    // Cheap bounding-radius early-out: avoids allocating world vertices when shapes are far apart
    float radiusSum = colA.radius + colB.radius;
    float distSq = (centerA - centerB).MagnitudeSquared();
    if (distSq >= radiusSum * radiusSum || distSq == 0.0f) {
      return;
    }
    std::vector<Vec3> vertsA = colA.GetWorldVertices(*a);
    std::vector<Vec3> vertsB = colB.GetWorldVertices(*b);
    Elysium::SATResult sat = Elysium::SAT::TestPolygonPolygon(vertsA, vertsB);
    if (sat.collided) {
      ApplyImpulse(a, b, sat.normal, sat.depth);
    }
  }

  static void ResolveSphereSphere(RigidBody *a, const Collider &colA,
                                  RigidBody *b, const Collider &colB) {
    Vec3 posA = a->LocalToGlobal(colA.localCentroid);
    Vec3 posB = b->LocalToGlobal(colB.localCentroid);
    float radiusA = colA.radius;
    float radiusB = colB.radius;

    Vec3 normal = posB - posA; // From A to B
    float distSq = normal.MagnitudeSquared();
    float radiusSum = radiusA + radiusB;

    if (distSq >= radiusSum * radiusSum || distSq == 0)
      return;

    float distance = std::sqrt(distSq);
    normal = normal / distance;

    ApplyImpulse(a, b, normal, radiusSum - distance);
  }

  
  static void ApplyImpulse(RigidBody *a, RigidBody *b,
                           const Vec3 &normalFromAtoB, float overlap) {
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

    // Do not resolve if velocities are separating (relative velocity from A to
    // B is positive)
    if (velAlongNormal > 0)
      return;

    float restitution = 0.8f;
    float j = -(1.0f + restitution) * velAlongNormal;
    j /= totalInvMass;

    Vec3 impulse = normalFromAtoB * j;
    a->linearVelocity -= impulse * a->inverseMass;
    b->linearVelocity += impulse * b->inverseMass;

    // 3. Friction Resolution (Tangent Impulse)
    Vec3 tangentVel = relativeVel - (normalFromAtoB * velAlongNormal);
    float tangentSpeed = tangentVel.Magnitude();

    if (tangentSpeed > EPSILON) {
      Vec3 tangent = tangentVel / tangentSpeed;
      float jt = -relativeVel.Dot(tangent);
      jt /= totalInvMass;

      // Coulomb's law: friction force <= friction_coeff * normal force
      // In impulse form: tangent_impulse <= friction_coeff * normal_impulse
      float combinedFriction = std::sqrt(a->friction * b->friction);
      float maxJt = combinedFriction * j;

      if (jt > maxJt)
        jt = maxJt;
      if (jt < -maxJt)
        jt = -maxJt;

      Vec3 frictionImpulse = tangent * jt;
      a->linearVelocity -= frictionImpulse * a->inverseMass;
      b->linearVelocity += frictionImpulse * b->inverseMass;
    }
  }
};

#endif
