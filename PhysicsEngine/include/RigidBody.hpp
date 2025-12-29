

struct RigidBody {
  float mass;
  float inverseMass;
  Mat3 localInverseInertiaTensor;
  Mat3 globalInverseInertiaTensor;

  Vec3 globalCentroid;
  Vec3 localCentroid;


  Vec3 position;
  Vec3 orientation;
  Vec3 linearVelocity;
  Vec3 angularVelocity;

  Vec3 forceAccumulator;
  Vec3 torqueAccumulator;

  ColliderList colliders;

  void UpdateGlobalCentroidFromPosition(void);
  void UpdatePositionFromGlobalCentroid(void);

  void UpdateOrientation(void);
  
  void AddCollider(Collider &collider);

  const Vec3 LocalToGlobal(const Vec3 &p) const;
  const Vec3 GlobalToLocal(const Vec3 &p) const;
  const Vec3 LocalToGlobalVec(const Vec3 &v) const;
  const Vec3 GlobalToLocalVec(const Vec3 &v) const;

  void ApplyForce(const Vec3 &f, const Vec3 &at);
}

struct Collider
{
  float m_mass;
  Mat3 m_localInertiaTensor;
  Vec3 m_localCentroid;
};
