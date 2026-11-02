#ifndef TRANSFORMCOMPONENT_HPP
#define TRANSFORMCOMPONENT_HPP

#include "Component.hpp"
#include "CoreMath.hpp"
#include "UUID.hpp"
#include <vector>
#include <memory>

class TransformComponent : public Component {
public:
  // Hierarchy
  Elysium::UUID ParentHandle = 0;
  std::vector<Elysium::UUID> ChildrenHandles;

  // Local Transform
  Vec3 position = Vec3(0.0f, 0.0f, 0.0f);
  Quat rotation = Quat();
  Vec3 scale = Vec3(1.0f, 1.0f, 1.0f);

  // Computed World Transform
  Mat4 worldMatrix = Mat4::Identity();
  Vec3 worldPosition = Vec3(0.0f, 0.0f, 0.0f);
  Quat worldRotation = Quat();
  Vec3 worldScale = Vec3(1.0f, 1.0f, 1.0f);
  bool isDirty = true;

  TransformComponent();
  TransformComponent(const Vec3 &pos, const Quat &rot = Quat(), const Vec3 &scl = Vec3(1.0f, 1.0f, 1.0f));

  // Local accessors
  const Vec3 &GetLocalPosition() const { return position; }
  void SetLocalPosition(const Vec3 &pos);

  const Quat &GetLocalRotation() const { return rotation; }
  void SetLocalRotation(const Quat &rot);

  const Vec3 &GetLocalScale() const { return scale; }
  void SetLocalScale(const Vec3 &scl);

  // World accessors
  const Vec3 &GetWorldPosition() const { return worldPosition; }
  const Quat &GetWorldRotation() const { return worldRotation; }
  const Vec3 &GetWorldScale() const { return worldScale; }
  const Mat4 &GetWorldMatrix() const { return worldMatrix; }

  Mat4 GetLocalMatrix() const;

  /**
   * @brief Recomputes world transform from parent matrix and recursively updates all children.
   */
  void UpdateWorldTransform(const Mat4 &parentWorldMatrix = Mat4::Identity());

  // Component clone
  std::unique_ptr<Component> Clone() const override;
  void OnCloned(const std::shared_ptr<GameObject> &owner) override;
};

namespace Elysium {
using TransformComponent = ::TransformComponent;
}

#endif // TRANSFORMCOMPONENT_HPP
