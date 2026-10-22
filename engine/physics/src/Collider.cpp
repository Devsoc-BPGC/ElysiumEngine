#include "Collider.hpp"
#include "RigidBody.hpp"
std::vector<Vec3> Collider::GetWorldVertices(const RigidBody& body) const {
  std::vector<Vec3> worldVerts;
  if (type == ColliderType::Box) {
    float hx = halfExtents.x;
    float hy = halfExtents.y;
    Vec3 corners[4] = {
      localCentroid + Vec3(-hx, -hy, 0.0f),
      localCentroid + Vec3( hx, -hy, 0.0f),
      localCentroid + Vec3( hx,  hy, 0.0f),
      localCentroid + Vec3(-hx,  hy, 0.0f)
    };
    for (int i = 0; i < 4; ++i) {
      worldVerts.push_back(body.LocalToGlobal(corners[i]));
    }
  } else if (type == ColliderType::Polygon) {
    for (const auto& v : localVertices) {
      worldVerts.push_back(body.LocalToGlobal(localCentroid + v));
    }
  }
  return worldVerts;
}