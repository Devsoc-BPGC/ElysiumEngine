#include "elypch.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"

GameObject::GameObject(const std::string &initialName)
    : m_UUID(), name(initialName), m_Scene(nullptr),
      position(0, 0, 0), rotation(Quat()), scale(1, 1, 1) {
  m_Transform.gameObject = this;
  m_Transform.position = position;
  m_Transform.rotation = rotation;
  m_Transform.scale = scale;
  m_Transform.UpdateWorldTransform();

  ELYSIUM_CORE_TRACE("GameObject created: '{}' [UUID: {}]", name,
                     static_cast<uint64_t>(m_UUID));
}

GameObject::GameObject(Elysium::UUID uuid, const std::string &initialName)
    : m_UUID(uuid), name(initialName), m_Scene(nullptr),
      position(0, 0, 0), rotation(Quat()), scale(1, 1, 1) {
  m_Transform.gameObject = this;
  m_Transform.position = position;
  m_Transform.rotation = rotation;
  m_Transform.scale = scale;
  m_Transform.UpdateWorldTransform();

  ELYSIUM_CORE_TRACE("GameObject created with explicit UUID: '{}' [UUID: {}]",
                     name, static_cast<uint64_t>(m_UUID));
}

void GameObject::SetParent(Elysium::UUID parentUUID) {
  if (m_Transform.ParentHandle == parentUUID)
    return;

  // Cannot parent to self
  if (parentUUID == m_UUID) {
    ELYSIUM_CORE_WARN("GameObject '{}': Cannot parent entity to itself", name);
    return;
  }

  // Cannot parent to one of our descendants (prevent cycles)
  if (parentUUID != 0 && IsParentOf(parentUUID)) {
    ELYSIUM_CORE_WARN("GameObject '{}': Cannot parent to a descendant (cycle detected)", name);
    return;
  }

  // If already parented to an entity, remove from old parent's children
  if (m_Transform.ParentHandle != 0 && m_Scene) {
    GameObject *oldParent = m_Scene->GetEntityByUUID(m_Transform.ParentHandle);
    if (oldParent) {
      oldParent->RemoveChild(m_UUID);
    }
  }

  m_Transform.ParentHandle = parentUUID;

  // If attaching to a new valid parent
  if (parentUUID != 0 && m_Scene) {
    GameObject *newParent = m_Scene->GetEntityByUUID(parentUUID);
    if (newParent) {
      auto &children = newParent->GetTransform().ChildrenHandles;
      if (std::find(children.begin(), children.end(), m_UUID) == children.end()) {
        children.push_back(m_UUID);
      }
    }
  }

  // Recalculate transforms down the branch
  if (m_Scene) {
    if (m_Transform.ParentHandle != 0) {
      GameObject *parent = m_Scene->GetEntityByUUID(m_Transform.ParentHandle);
      if (parent) {
        m_Transform.UpdateWorldTransform(parent->GetWorldTransform());
      } else {
        m_Transform.UpdateWorldTransform(Mat4::Identity());
      }
    } else {
      m_Transform.UpdateWorldTransform(Mat4::Identity());
    }
  } else {
    m_Transform.UpdateWorldTransform(Mat4::Identity());
  }

  SyncTransform();
}

void GameObject::RemoveChild(Elysium::UUID childUUID) {
  auto &children = m_Transform.ChildrenHandles;
  auto it = std::find(children.begin(), children.end(), childUUID);
  if (it != children.end()) {
    children.erase(it);
    if (m_Scene) {
      GameObject *child = m_Scene->GetEntityByUUID(childUUID);
      if (child && child->GetTransform().ParentHandle == m_UUID) {
        child->GetTransform().ParentHandle = 0;
        child->GetTransform().UpdateWorldTransform(Mat4::Identity());
        child->SyncTransform();
      }
    }
  }
}

bool GameObject::IsParentOf(Elysium::UUID entityUUID) const {
  if (entityUUID == 0 || !m_Scene)
    return false;

  for (Elysium::UUID childUUID : m_Transform.ChildrenHandles) {
    if (childUUID == entityUUID)
      return true;

    GameObject *child = m_Scene->GetEntityByUUID(childUUID);
    if (child && child->IsParentOf(entityUUID))
      return true;
  }
  return false;
}

std::shared_ptr<GameObject> GameObject::Clone() const {
  auto copy = std::make_shared<GameObject>(m_UUID, name);
  copy->active = active;
  copy->position = position;
  copy->rotation = rotation;
  copy->scale = scale;
  copy->m_Scene = m_Scene;

  copy->m_Transform.position = position;
  copy->m_Transform.rotation = rotation;
  copy->m_Transform.scale = scale;
  copy->m_Transform.ParentHandle = m_Transform.ParentHandle;
  copy->m_Transform.ChildrenHandles = m_Transform.ChildrenHandles;
  copy->m_Transform.worldMatrix = m_Transform.worldMatrix;
  copy->m_Transform.worldPosition = m_Transform.worldPosition;
  copy->m_Transform.worldRotation = m_Transform.worldRotation;
  copy->m_Transform.worldScale = m_Transform.worldScale;
  copy->m_Transform.isDirty = m_Transform.isDirty;
  copy->m_Transform.gameObject = copy.get();

  if (rigidBody) {
    copy->rigidBody = std::make_unique<RigidBody>(*rigidBody);
  }
  for (const auto &comp : components) {
    if (auto compCopy = comp->Clone()) {
      compCopy->gameObject = copy.get();
      copy->components.push_back(std::move(compCopy));
    }
  }
  for (auto &comp : copy->components) {
    comp->OnCloned(copy);
  }
  return copy;
}

void GameObject::SyncPhysics() {
  if (rigidBody) {
    // If unparented, local == world position directly
    if (m_Transform.ParentHandle == 0 || !m_Scene) {
      position = rigidBody->position;
      rotation = rigidBody->orientation.ToQuat();
    } else {
      // If parented, compute local position from world position and parent's world matrix
      GameObject *parent = m_Scene->GetEntityByUUID(m_Transform.ParentHandle);
      if (parent) {
        Vec3 parentPos = parent->GetWorldPosition();
        Quat parentRot = parent->GetWorldRotation();
        Quat parentRotInv = parentRot.Conjugate();
        Vec3 localPos = parentRotInv.RotateVector(rigidBody->position - parentPos);
        localPos.x /= (parent->GetWorldScale().x > EPSILON ? parent->GetWorldScale().x : 1.0f);
        localPos.y /= (parent->GetWorldScale().y > EPSILON ? parent->GetWorldScale().y : 1.0f);
        localPos.z /= (parent->GetWorldScale().z > EPSILON ? parent->GetWorldScale().z : 1.0f);
        position = localPos;
        rotation = parentRotInv * rigidBody->orientation.ToQuat();
      } else {
        position = rigidBody->position;
        rotation = rigidBody->orientation.ToQuat();
      }
    }
    m_Transform.position = position;
    m_Transform.rotation = rotation;
    m_Transform.scale = scale;
    // Update world transform
    if (m_Transform.ParentHandle != 0 && m_Scene) {
      GameObject *parent = m_Scene->GetEntityByUUID(m_Transform.ParentHandle);
      if (parent) {
        m_Transform.UpdateWorldTransform(parent->GetWorldTransform());
      } else {
        m_Transform.UpdateWorldTransform(Mat4::Identity());
      }
    } else {
      m_Transform.UpdateWorldTransform(Mat4::Identity());
    }
  }
}

void GameObject::SyncTransform() {
  m_Transform.position = position;
  m_Transform.rotation = rotation;
  m_Transform.scale = scale;

  // Determine parent matrix
  Mat4 parentMatrix = Mat4::Identity();
  if (m_Transform.ParentHandle != 0 && m_Scene) {
    GameObject *parent = m_Scene->GetEntityByUUID(m_Transform.ParentHandle);
    if (parent) {
      parentMatrix = parent->GetWorldTransform();
    }
  }

  m_Transform.UpdateWorldTransform(parentMatrix);

  if (rigidBody) {
    rigidBody->position = GetWorldPosition();
    rigidBody->orientation = GetWorldRotation().ToMatrix();
    rigidBody->UpdateGlobalCentroidFromPosition();
  }
}
