#include "ScriptingWorld.hpp"
#include "Scene.hpp"
#include "GameObject.hpp"
#include "RigidBody.hpp"
#include "Collider.hpp"
#include "SpriteRenderer.hpp"
#include "Log.h"

namespace Elysium {

static Scene* s_ActiveScene = nullptr;

/**
 * @brief Sets the active scene.
 * @param scene Scene pointer.
 */
void SetActiveScene(Scene* scene) {
    s_ActiveScene = scene;
}

/**
 * @brief Gets the active scene.
 * @return Active Scene pointer.
 */
Scene* GetActiveScene() {
    return s_ActiveScene;
}

/**
 * @brief Spawns a dynamic box entity.
 * @param name Entity name.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param width Box width.
 * @param height Box height.
 * @param density Mass density.
 * @return Spawned Entity.
 */
Entity SpawnBox(const std::string& name, float x, float y, float width, float height, float density) {
    if (!s_ActiveScene) {
        ELYSIUM_CORE_ERROR("SpawnBox failed: No active scene set. Call SetActiveScene() first.");
        return Entity();
    }

    auto go = std::make_shared<GameObject>(name);
    go->position = Vec3(x, y, 0.0f);

    auto& rb = go->CreateRigidBody();
    rb.isStatic = (density <= 0.0f);
    rb.restitution = 0.5f;
    rb.friction = 0.3f;
    rb.AddColliders(Collider::CreateBox(Vec3(width * 0.5f, height * 0.5f, 0.0f), density));

    go->AddComponent<SpriteRenderer>(sf::Color(160, 100, 220), sf::Vector2f(width, height));
    s_ActiveScene->AddGameObject(go);

    return Entity(go);
}

/**
 * @brief Spawns a static box entity.
 * @param name Entity name.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param width Box width.
 * @param height Box height.
 * @return Spawned Entity.
 */
Entity SpawnStaticBox(const std::string& name, float x, float y, float width, float height) {
    Entity box = SpawnBox(name, x, y, width, height, 0.0f);
    box.SetStatic(true);
    box.SetColor(ColorRGBA::Green());
    return box;
}

/**
 * @brief Spawns a dynamic circle entity.
 * @param name Entity name.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param radius Circle radius.
 * @param density Mass density.
 * @return Spawned Entity.
 */
Entity SpawnCircle(const std::string& name, float x, float y, float radius, float density) {
    if (!s_ActiveScene) {
        ELYSIUM_CORE_ERROR("SpawnCircle failed: No active scene set. Call SetActiveScene() first.");
        return Entity();
    }

    auto go = std::make_shared<GameObject>(name);
    go->position = Vec3(x, y, 0.0f);

    auto& rb = go->CreateRigidBody();
    rb.isStatic = (density <= 0.0f);
    rb.restitution = 0.75f;
    rb.friction = 0.2f;
    rb.AddColliders(Collider::CreateSphere(radius, density));

    go->AddComponent<SpriteRenderer>(sf::Color(240, 80, 80), radius);
    s_ActiveScene->AddGameObject(go);

    return Entity(go);
}

/**
 * @brief Spawns a static circle entity.
 * @param name Entity name.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param radius Circle radius.
 * @return Spawned Entity.
 */
Entity SpawnStaticCircle(const std::string& name, float x, float y, float radius) {
    Entity circle = SpawnCircle(name, x, y, radius, 0.0f);
    circle.SetStatic(true);
    circle.SetColor(ColorRGBA::Blue());
    return circle;
}

/**
 * @brief Finds an entity by name.
 * @param name Entity name.
 * @return Entity handle.
 */
Entity FindEntity(const std::string& name) {
    if (!s_ActiveScene) return Entity();

    for (const auto& obj : s_ActiveScene->objects) {
        if (obj && obj->active && obj->name == name) {
            return Entity(obj);
        }
    }
    return Entity();
}

/**
 * @brief Gets all active entities in the scene.
 * @return Vector of Entity handles.
 */
std::vector<Entity> GetAllEntities() {
    std::vector<Entity> result;
    if (!s_ActiveScene) return result;

    result.reserve(s_ActiveScene->objects.size());
    for (const auto& obj : s_ActiveScene->objects) {
        if (obj && obj->active) {
            result.emplace_back(obj);
        }
    }
    return result;
}

/**
 * @brief Sets world gravity.
 * @param gx Gravity X component.
 * @param gy Gravity Y component.
 */
void SetGravity(float gx, float gy) {
    if (s_ActiveScene) {
        s_ActiveScene->physicsWorld.gravity = Vec3(gx, gy, 0.0f);
    }
}

/**
 * @brief Sets world gravity.
 * @param gravity Gravity vector.
 */
void SetGravity(const Vector2& gravity) {
    SetGravity(gravity.x, gravity.y);
}

/**
 * @brief Sets outer boundary limits.
 * @param centerX Boundary center X.
 * @param centerY Boundary center Y.
 * @param halfWidth Boundary half-width.
 * @param halfHeight Boundary half-height.
 */
void SetWorldBounds(float centerX, float centerY, float halfWidth, float halfHeight) {
    if (s_ActiveScene) {
        s_ActiveScene->physicsWorld.SetBoundaries(
            Vec3(centerX, centerY, 0.0f),
            Vec3(halfWidth, halfHeight, 0.0f)
        );
    }
}

} // namespace Elysium
