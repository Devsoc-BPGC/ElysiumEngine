#include "elypch.hpp"
#include "Scene.hpp"
#include "CameraComponent.hpp"

Scene::Scene() : physicsWorld(Vec3(0, -9.81f, 0)) {}

void Scene::AddGameObject(std::shared_ptr<GameObject> obj) {
  if (!obj)
    return;

  obj->SetScene(this);
  objects.push_back(obj);
  m_EntityMap[obj->GetUUID()] = obj;

  if (obj->rigidBody) {
    physicsWorld.AddBody(obj->rigidBody.get());
  }

  // Ensure world transform is initialized with hierarchy
  if (obj->GetTransform().ParentHandle != 0) {
    GameObject *parent = GetEntityByUUID(obj->GetTransform().ParentHandle);
    if (parent) {
      obj->GetTransform().UpdateWorldTransform(parent->GetWorldTransform());
    } else {
      obj->GetTransform().UpdateWorldTransform(Mat4::Identity());
    }
  } else {
    obj->GetTransform().UpdateWorldTransform(Mat4::Identity());
  }
  obj->SyncTransform();

  ELYSIUM_CORE_TRACE("Entity registered to Scene: '{}' [UUID: {}]", obj->name,
                     static_cast<uint64_t>(obj->GetUUID()));
}

GameObject *Scene::GetEntityByUUID(Elysium::UUID uuid) {
  auto it = m_EntityMap.find(uuid);
  if (it != m_EntityMap.end()) {
    return it->second.get();
  }
  return nullptr;
}

void Scene::AddTexture(std::shared_ptr<const Texture> texture) {
  if (!texture)
    return;

  textures.push_back(texture);
}

bool Scene::DestroyEntityByUUID(Elysium::UUID uuid) {
  auto it = m_EntityMap.find(uuid);
  if (it == m_EntityMap.end()) {
    ELYSIUM_CORE_WARN("DestroyEntityByUUID: Entity with UUID {} not found",
                      static_cast<uint64_t>(uuid));
    return false;
  }

  std::shared_ptr<GameObject> entity = it->second;

  // 1. Recursively destroy all children first
  if (entity) {
    // Copy children list to avoid modifying during iteration
    auto children = entity->GetTransform().ChildrenHandles;
    for (Elysium::UUID childUUID : children) {
      DestroyEntityByUUID(childUUID);
    }

    // 2. Unparent from parent if parented
    if (entity->GetTransform().ParentHandle != 0) {
      GameObject *parent = GetEntityByUUID(entity->GetTransform().ParentHandle);
      if (parent) {
        auto &parentChildren = parent->GetTransform().ChildrenHandles;
        parentChildren.erase(std::remove(parentChildren.begin(), parentChildren.end(), uuid),
                             parentChildren.end());
      }
    }

    entity->active = false;
    if (entity->rigidBody) {
      physicsWorld.RemoveBody(entity->rigidBody.get());
    }
  }

  m_EntityMap.erase(it);

  auto vecIt = std::find_if(objects.begin(), objects.end(),
                            [uuid](const std::shared_ptr<GameObject> &o) {
                              return o && o->GetUUID() == uuid;
                            });
  if (vecIt != objects.end()) {
    objects.erase(vecIt);
  }

  ELYSIUM_CORE_INFO("Destroyed entity [UUID: {}]", static_cast<uint64_t>(uuid));
  return true;
}

std::unique_ptr<Scene> Scene::Clone() const {
  auto copy = std::make_unique<Scene>();
  copy->textures = textures; // read-only, so both scenes can share them
  copy->physicsWorld = physicsWorld; // gravity, boundary, etc.
  copy->physicsWorld.rigidBodies.clear();

  std::unordered_map<const RigidBody *, RigidBody *> bodyCopies;
  for (const auto &obj : objects) {
    if (!obj)
      continue;
    auto objCopy = obj->Clone();
    objCopy->SetScene(copy.get());
    if (obj->rigidBody) {
      bodyCopies[obj->rigidBody.get()] = objCopy->rigidBody.get();
    }
    copy->m_EntityMap[objCopy->GetUUID()] = objCopy;
    copy->objects.push_back(std::move(objCopy));
  }

  // Register bodies in the same order as this scene, so the copy simulates
  // exactly as this scene would
  for (const RigidBody *body : physicsWorld.rigidBodies) {
    auto it = bodyCopies.find(body);
    if (it != bodyCopies.end()) {
      copy->physicsWorld.AddBody(it->second);
    }
  }

  // Update world transforms for all entities in the cloned scene
  copy->UpdateTransforms();

  return copy;
}

void Scene::UpdateTransforms() {
  for (auto &obj : objects) {
    if (obj && obj->active && obj->GetTransform().ParentHandle == 0) {
      obj->SyncTransform();
    }
  }
}

void Scene::Update(float dt) {
  // 1. Update transforms across scene graph
  UpdateTransforms();

  // 2. Update components
  for (auto &obj : objects) {
    if (obj->active)
      obj->Update(dt);
  }

  // 3. Cleanup inactive objects
  auto it = objects.begin();
  while (it != objects.end()) {
    if (!(*it)->active) {
      if ((*it)->rigidBody) {
        physicsWorld.RemoveBody((*it)->rigidBody.get());
      }
      m_EntityMap.erase((*it)->GetUUID());
      it = objects.erase(it);
    } else {
      ++it;
    }
  }

  // 4. Step physics
  physicsWorld.Step(dt);

  // 5. Sync game objects with physics results
  for (auto &obj : objects) {
    obj->SyncPhysics();
  }

  // 6. Keep world matrices synchronized after physics integration
  UpdateTransforms();
}

void Scene::Clear() {
  for (auto &obj : objects) {
    if (obj && obj->rigidBody) {
      physicsWorld.RemoveBody(obj->rigidBody.get());
    }
  }

  objects.clear();
  m_EntityMap.clear();

  ELYSIUM_CORE_INFO("Scene cleared");
}

GameObject *Scene::GetPrimaryCameraEntity() {
  for (auto &obj : objects) {
    if (obj && obj->active) {
      if (auto *cam = obj->GetComponent<Elysium::CameraComponent>()) {
        if (cam->Primary) {
          return obj.get();
        }
      }
    }
  }
  return nullptr;
}

const GameObject *Scene::GetPrimaryCameraEntity() const {
  for (const auto &obj : objects) {
    if (obj && obj->active) {
      if (const auto *cam = const_cast<GameObject *>(obj.get())->GetComponent<Elysium::CameraComponent>()) {
        if (cam->Primary) {
          return obj.get();
        }
      }
    }
  }
  return nullptr;
}

void Scene::OnViewportResize(uint32_t width, uint32_t height) {
  for (auto &obj : objects) {
    if (obj && obj->active) {
      if (auto *cam = obj->GetComponent<Elysium::CameraComponent>()) {
        if (!cam->FixedAspectRatio) {
          cam->Camera.SetViewportSize(width, height);
        }
      }
    }
  }
}

Mat4 Scene::GetPrimaryCameraViewProjection() const {
  const GameObject *camEntity = GetPrimaryCameraEntity();
  if (!camEntity) {
    return Mat4::Identity();
  }

  const auto *cam = const_cast<GameObject *>(camEntity)->GetComponent<Elysium::CameraComponent>();
  if (!cam) {
    return Mat4::Identity();
  }

  Vec3 pos = camEntity->GetWorldPosition();
  Quat rot = camEntity->GetWorldRotation();

  Mat4 view = Mat4::Rotate(rot.Conjugate()) * Mat4::Translate(-pos);
  return cam->Camera.GetProjection() * view;
}

