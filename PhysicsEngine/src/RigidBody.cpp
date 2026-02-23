#include "../include/RigidBody.hpp"
#include "../include/CoreMath.hpp"

void RigidBody::UpdateGlobalCentroidFromPosition(void) {
  globalCentroid = orientation * localCentroid + position;
}

void RigidBody::UpdatePositionFromGlobalCentroid(void) {
  position = orientation * (-localCentroid) + globalCentroid;
}

void RigidBody::AddColliders(Collider &collider) {
  colliders.push_back(collider);

  localCentroid.Zero();
  mass = 0.0f;

  for(Collider &collider : colliders) {
    mass += collider.mass;
    localCentroid += collider.mass * collider.localCentroid;
  }

  inverseMass = 1.0f / mass;
  localCentroid *= inverseMass;

  Mat3 localInertiaTensor;
  localInertiaTensor.Zero();

  for (Collider &collider : colliders) {
    const Vec3 r = localCentroid - collider.localCentroid;
    const float rDotR = r.Dot(r);
    const Mat3 rOutR = r.outerProduct(r);

    localInertiaTensor += collider.localInertiaTensor + collider.mass * (rDotR * Mat3::Identity() - rOutR);
  }

  localInverseInertiaTensor = localInertiaTensor.Inverted();
}

const Vec3 RigidBody::LocalToGlobal(const Vec3 &p) const
{
  return orientation * p + position;
}

const Vec3 RigidBody::GlobalToLocal(const Vec3 &p) const
{
  return inverseOrientation * (p - position);
}

const Vec3 RigidBody::LocalToGlobalVec(const Vec3 &v) const
{
  return orientation * v;
}

const Vec3 RigidBody::GlobalToLocalVec(const Vec3 &v) const
{
  return inverseOrientation * v;
}

void RigidBody::ApplyForce(const Vec3 &f, const Vec3 &at) {
  forceAccumulator += f;
  torqueAccumulator += (at - globalCentroid).Cross(f);
}

void RigidBody::UpdateOrientation(void) {
  Quat q = orientation.ToQuat();
  q.Normalize();
  orientation = q.ToMatrix();

  inverseOrientation = orientation.Transposed();
}

void RigidBody::Integrate(float dt) {
    if (inverseMass <= 0.0f) return;
    Vec3 linearAcceleration = forceAccumulator * inverseMass;
    linearVelocity += linearAcceleration * dt;

    Vec3 angularAcceleration = inverseInertiaTensorWorld * torqueAccumulator;
    angularVelocity += angularAcceleration * dt;

    forceAccumulator.Zero();
    torqueAccumulator.Zero();

    globalCentroid += linearVelocity * dt;


    if (angularVelocity.MagnitudeSquared() > EPSILON) {
        Vec3 axis = angularVelocity.Normalized();
        float angle = angularVelocity.Magnitude() * dt;
        orientation = Mat3::RotationMatrix(axis, angle) * orientation;
    }

    UpdateOrientation();
    UpdatePositionFromGlobalCentroid();
}
