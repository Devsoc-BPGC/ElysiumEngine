/**
 * @file Game.hpp
 * @brief Public, game-facing helpers built on the engine's existing types.
 *
 * Include this file from gameplay code. Low-level systems remain available
 * through their existing headers for engine and editor implementation.
 */
#pragma once

#include "GameObject.hpp"
#include "Scene.hpp"
#include "SpriteRenderer.hpp"

#include <memory>
#include <string>
#include <utility>

namespace Elysium::Game {

/** A shared, lifetime-safe handle to a game object. An empty handle is invalid. */
class Entity {
public:
  Entity() = default;
  explicit Entity(std::shared_ptr<GameObject> object) : m_Object(std::move(object)) {}

  explicit operator bool() const noexcept { return static_cast<bool>(m_Object); }
  bool IsValid() const noexcept { return static_cast<bool>(m_Object); }

  uint32_t GetId() const noexcept { return m_Object ? m_Object->id : 0; }
  std::string GetName() const { return m_Object ? m_Object->name : std::string{}; }
  bool SetName(const std::string &name) {
    if (!m_Object) return false;
    m_Object->name = name;
    return true;
  }
  bool IsActive() const noexcept { return m_Object && m_Object->active; }
  bool SetActive(bool active) {
    if (!m_Object) return false;
    m_Object->active = active;
    return true;
  }

  bool GetPosition(Vec3 &out) const { if (!m_Object) return false; out = m_Object->position; return true; }
  bool SetPosition(const Vec3 &value) { if (!m_Object) return false; m_Object->position = value; m_Object->SyncTransform(); return true; }
  bool GetRotation(Quat &out) const { if (!m_Object) return false; out = m_Object->rotation; return true; }
  bool SetRotation(const Quat &value) { if (!m_Object) return false; m_Object->rotation = value; m_Object->SyncTransform(); return true; }
  bool GetScale(Vec3 &out) const { if (!m_Object) return false; out = m_Object->scale; return true; }
  bool SetScale(const Vec3 &value) { if (!m_Object) return false; m_Object->scale = value; return true; }

  template <typename T, typename... Args> T *AddComponent(Args &&...args) {
    return m_Object ? &m_Object->AddComponent<T>(std::forward<Args>(args)...) : nullptr;
  }
  template <typename T> T *GetComponent() const { return m_Object ? m_Object->GetComponent<T>() : nullptr; }
  template <typename T> bool HasComponent() const { return m_Object && m_Object->HasComponent<T>(); }

  RigidBody *GetRigidBody() const noexcept { return m_Object ? m_Object->rigidBody.get() : nullptr; }
  RigidBody *CreateRigidBody() { return m_Object ? &m_Object->CreateRigidBody() : nullptr; }
  bool ApplyForce(const Vec3 &force, const Vec3 &worldPoint) {
    auto *body = GetRigidBody();
    if (!body) return false;
    body->ApplyForce(force, worldPoint);
    return true;
  }
  bool AddCollider(const Collider &collider) {
    auto *body = GetRigidBody();
    if (!body) return false;
    body->AddColliders(collider);
    return true;
  }

private:
  std::shared_ptr<GameObject> m_Object;
  friend class Scene;
};

/** Thin game-facing access to a scene. Create physics bodies before Add(). */
class Scene {
public:
  explicit Scene(::Scene &scene) noexcept : m_Scene(scene) {}

  Entity CreateEntity(const std::string &name = "New GameObject") {
    return Entity(std::make_shared<GameObject>(name));
  }
  bool Add(const Entity &entity) {
    if (!entity.m_Object) return false;
    m_Scene.AddGameObject(entity.m_Object);
    return true;
  }
  Entity FindByName(const std::string &name) const {
    for (const auto &object : m_Scene.objects)
      if (object && object->name == name) return Entity(object);
    return {};
  }
  std::size_t GetEntityCount() const noexcept { return m_Scene.objects.size(); }

private:
  ::Scene &m_Scene;
};

} // namespace Elysium::Game
