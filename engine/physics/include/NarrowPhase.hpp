#ifndef NARROW_PHASE_HPP
#define NARROW_PHASE_HPP

#include "RigidBody.hpp"
#include "SAT.hpp"
#include <cmath>


struct NarrowPhaseStats {
  size_t candidatePairsTested = 0;
  size_t boundingRadiusPruned = 0;
  size_t satChecksPerformed = 0;
  size_t actualCollisions = 0;
};

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
   * @param stats Optional pointer to collect narrowphase collision statistics.
   */
  static bool IsConvexShape(ColliderType type) {
    return type == ColliderType::Box || type == ColliderType::Polygon;
  }

  static void ResolveCollision(RigidBody *a, RigidBody *b, NarrowPhaseStats *stats = nullptr) {
    for (auto &colA : a->colliders) {
      for (auto &colB : b->colliders) {
        if (stats) {
          stats->candidatePairsTested++;
        }
        bool isPolyA = IsConvexShape(colA.type);
        bool isPolyB = IsConvexShape(colB.type);

        if (colA.type == ColliderType::Sphere && colB.type == ColliderType::Sphere) {
          ResolveSphereSphere(a, colA, b, colB, stats);
        } else if (colA.type == ColliderType::Sphere && isPolyB) {
          ResolveCirclePolygon(a, colA, b, colB, stats);
        } else if (isPolyA && colB.type == ColliderType::Sphere) {
          ResolveCirclePolygon(b, colB, a, colA, stats);
        } else if (isPolyA && isPolyB) {
          ResolvePolygonPolygon(a, colA, b, colB, stats);
        }
      }
    }
  }

  /**
   * @brief Performs narrowphase collision detection between two bodies without applying impulses.
   * @return true if an actual collision was detected, false otherwise.
   */
  static bool TestCollision(RigidBody *a, RigidBody *b, Elysium::SATResult *outResult = nullptr,
                            NarrowPhaseStats *stats = nullptr) {
    bool hasCollision = false;
    for (auto &colA : a->colliders) {
      for (auto &colB : b->colliders) {
        if (stats) {
          stats->candidatePairsTested++;
        }
        bool isPolyA = IsConvexShape(colA.type);
        bool isPolyB = IsConvexShape(colB.type);

        if (colA.type == ColliderType::Sphere && colB.type == ColliderType::Sphere) {
          Vec3 posA = a->LocalToGlobal(colA.localCentroid);
          Vec3 posB = b->LocalToGlobal(colB.localCentroid);
          float radiusSum = colA.radius + colB.radius;
          Vec3 diff = posB - posA;
          float distSq = diff.MagnitudeSquared();
          if (distSq >= radiusSum * radiusSum || distSq == 0.0f) {
            if (stats) stats->boundingRadiusPruned++;
            continue;
          }
          float dist = std::sqrt(distSq);
          if (stats) stats->actualCollisions++;
          if (outResult) {
            outResult->collided = true;
            outResult->normal = diff / dist;
            outResult->depth = radiusSum - dist;
          }
          hasCollision = true;
        } else if (colA.type == ColliderType::Sphere && isPolyB) {
          Vec3 circleCenter = a->LocalToGlobal(colA.localCentroid);
          Vec3 polyCenter = b->LocalToGlobal(colB.localCentroid);
          float radiusSum = colA.radius + colB.radius;
          float distSq = (circleCenter - polyCenter).MagnitudeSquared();
          if (distSq >= radiusSum * radiusSum || distSq == 0.0f) {
            if (stats) stats->boundingRadiusPruned++;
            continue;
          }
          if (stats) stats->satChecksPerformed++;
          std::vector<Vec3> polyVerts = colB.GetWorldVertices(*b);
          Elysium::SATResult sat = Elysium::SAT::TestCirclePolygon(circleCenter, colA.radius, polyVerts);
          if (sat.collided) {
            if (stats) stats->actualCollisions++;
            if (outResult) *outResult = sat;
            hasCollision = true;
          }
        } else if (isPolyA && colB.type == ColliderType::Sphere) {
          Vec3 circleCenter = b->LocalToGlobal(colB.localCentroid);
          Vec3 polyCenter = a->LocalToGlobal(colA.localCentroid);
          float radiusSum = colA.radius + colB.radius;
          float distSq = (circleCenter - polyCenter).MagnitudeSquared();
          if (distSq >= radiusSum * radiusSum || distSq == 0.0f) {
            if (stats) stats->boundingRadiusPruned++;
            continue;
          }
          if (stats) stats->satChecksPerformed++;
          std::vector<Vec3> polyVerts = colA.GetWorldVertices(*a);
          Elysium::SATResult sat = Elysium::SAT::TestCirclePolygon(circleCenter, colB.radius, polyVerts);
          if (sat.collided) {
            if (stats) stats->actualCollisions++;
            if (outResult) {
              outResult->collided = true;
              outResult->normal = -sat.normal;
              outResult->depth = sat.depth;
            }
            hasCollision = true;
          }
        } else if (isPolyA && isPolyB) {
          Vec3 centerA = a->LocalToGlobal(colA.localCentroid);
          Vec3 centerB = b->LocalToGlobal(colB.localCentroid);
          float radiusSum = colA.radius + colB.radius;
          float distSq = (centerA - centerB).MagnitudeSquared();
          if (distSq >= radiusSum * radiusSum || distSq == 0.0f) {
            if (stats) stats->boundingRadiusPruned++;
            continue;
          }
          if (stats) stats->satChecksPerformed++;
          std::vector<Vec3> vertsA = colA.GetWorldVertices(*a);
          std::vector<Vec3> vertsB = colB.GetWorldVertices(*b);
          Elysium::SATResult sat = Elysium::SAT::TestPolygonPolygon(vertsA, vertsB);
          if (sat.collided) {
            if (stats) stats->actualCollisions++;
            if (outResult) *outResult = sat;
            hasCollision = true;
          }
        }
      }
    }
    return hasCollision;
  }

private:

  static void ResolveCirclePolygon(RigidBody *circleBody, const Collider &colCircle,
                                   RigidBody *polyBody, const Collider &colPoly,
                                   NarrowPhaseStats *stats = nullptr) {
    Vec3 circleCenter = circleBody->LocalToGlobal(colCircle.localCentroid);
    Vec3 polyCenter = polyBody->LocalToGlobal(colPoly.localCentroid);
    // Cheap bounding-radius early-out: avoids allocating world vertices when shapes are far apart
    float radiusSum = colCircle.radius + colPoly.radius;
    float distSq = (circleCenter - polyCenter).MagnitudeSquared();
    if (distSq >= radiusSum * radiusSum || distSq == 0.0f) {
      if (stats) stats->boundingRadiusPruned++;
      return;
    }
    if (stats) stats->satChecksPerformed++;
    std::vector<Vec3> polyVerts = colPoly.GetWorldVertices(*polyBody);
    Elysium::SATResult sat = Elysium::SAT::TestCirclePolygon(circleCenter, colCircle.radius, polyVerts);
    if (sat.collided) {
      if (stats) stats->actualCollisions++;
      ApplyImpulse(circleBody, polyBody, sat.normal, sat.depth);
    }
  }

  static void ResolvePolygonPolygon(RigidBody *a, const Collider &colA,
                                    RigidBody *b, const Collider &colB,
                                    NarrowPhaseStats *stats = nullptr) {
    Vec3 centerA = a->LocalToGlobal(colA.localCentroid);
    Vec3 centerB = b->LocalToGlobal(colB.localCentroid);
    // Cheap bounding-radius early-out: avoids allocating world vertices when shapes are far apart
    float radiusSum = colA.radius + colB.radius;
    float distSq = (centerA - centerB).MagnitudeSquared();
    if (distSq >= radiusSum * radiusSum || distSq == 0.0f) {
      if (stats) stats->boundingRadiusPruned++;
      return;
    }
    if (stats) stats->satChecksPerformed++;
    std::vector<Vec3> vertsA = colA.GetWorldVertices(*a);
    std::vector<Vec3> vertsB = colB.GetWorldVertices(*b);
    Elysium::SATResult sat = Elysium::SAT::TestPolygonPolygon(vertsA, vertsB);
    if (sat.collided) {
      if (stats) stats->actualCollisions++;
      ApplyImpulse(a, b, sat.normal, sat.depth);
    }
  }

  static void ResolveSphereSphere(RigidBody *a, const Collider &colA,
                                  RigidBody *b, const Collider &colB,
                                  NarrowPhaseStats *stats = nullptr) {
    Vec3 posA = a->LocalToGlobal(colA.localCentroid);
    Vec3 posB = b->LocalToGlobal(colB.localCentroid);
    float radiusA = colA.radius;
    float radiusB = colB.radius;

    Vec3 normal = posB - posA; // From A to B
    float distSq = normal.MagnitudeSquared();
    float radiusSum = radiusA + radiusB;

    if (distSq >= radiusSum * radiusSum || distSq == 0) {
      if (stats) stats->boundingRadiusPruned++;
      return;
    }

    if (stats) stats->actualCollisions++;
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
