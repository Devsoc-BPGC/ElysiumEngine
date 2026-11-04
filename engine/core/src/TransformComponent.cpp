#include "TransformComponent.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"

TransformComponent::TransformComponent()
    : position(0.0f, 0.0f, 0.0f), rotation(Quat()), scale(1.0f, 1.0f, 1.0f),
      worldMatrix(Mat4::Identity()), worldPosition(0.0f, 0.0f, 0.0f),
      worldRotation(Quat()), worldScale(1.0f, 1.0f, 1.0f), isDirty(true) {}

TransformComponent::TransformComponent(const Vec3 &pos, const Quat &rot,
                                       const Vec3 &scl)
    : position(pos), rotation(rot), scale(scl), worldMatrix(Mat4::Identity()),
      worldPosition(pos), worldRotation(rot), worldScale(scl), isDirty(true) {}

void TransformComponent::SetLocalPosition(const Vec3 &pos) {
  position = pos;
  isDirty = true;
  if (gameObject) {
    gameObject->position = pos;
    gameObject->SyncTransform();
  }
}

void TransformComponent::SetLocalRotation(const Quat &rot) {
  rotation = rot;
  isDirty = true;
  if (gameObject) {
    gameObject->rotation = rot;
    gameObject->SyncTransform();
  }
}

void TransformComponent::SetLocalScale(const Vec3 &scl) {
  scale = scl;
  isDirty = true;
  if (gameObject) {
    gameObject->scale = scl;
  }
}

Mat4 TransformComponent::GetLocalMatrix() const {
  return Mat4::TRS(position, rotation, scale);
}

void TransformComponent::UpdateWorldTransform(const Mat4 &parentWorldMatrix) {
  Mat4 localMat = GetLocalMatrix();
  worldMatrix = parentWorldMatrix * localMat;

  worldPosition = worldMatrix.GetTranslation();
  worldRotation = worldMatrix.GetRotation();
  worldScale = worldMatrix.GetScale();
  isDirty = false;

  // Propagate to child game objects
  if (gameObject && gameObject->GetScene()) {
    Scene *scene = gameObject->GetScene();
    for (Elysium::UUID childUUID : ChildrenHandles) {
      GameObject *child = scene->GetEntityByUUID(childUUID);
      if (child) {
        child->GetTransform().UpdateWorldTransform(worldMatrix);
        // If child has physics body, keep it synced with world transform
        if (child->rigidBody) {
          child->rigidBody->position = child->GetTransform().GetWorldPosition();
          child->rigidBody->orientation = child->GetTransform().GetWorldRotation().ToMatrix();
          child->rigidBody->UpdateGlobalCentroidFromPosition();
        }
      }
    }
  }
}

std::unique_ptr<Component> TransformComponent::Clone() const {
  auto copy = std::make_unique<TransformComponent>(position, rotation, scale);
  copy->ParentHandle = ParentHandle;
  copy->ChildrenHandles = ChildrenHandles;
  copy->worldMatrix = worldMatrix;
  copy->worldPosition = worldPosition;
  copy->worldRotation = worldRotation;
  copy->worldScale = worldScale;
  copy->isDirty = isDirty;
  return copy;
}

void TransformComponent::OnCloned(const std::shared_ptr<GameObject> &owner) {
  gameObject = owner.get();
}
