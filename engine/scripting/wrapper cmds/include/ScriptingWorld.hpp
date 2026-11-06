#pragma once

#include "Core.hpp"
#include "Entity.hpp"
#include "ScriptTypes.hpp"
#include <string>
#include <vector>

class Scene;

namespace Elysium {

/**
 * @brief Sets the active scene.
 * @param scene Scene pointer.
 */
void SetActiveScene(Scene* scene);

/**
 * @brief Gets the active scene.
 * @return Active Scene pointer.
 */
Scene* GetActiveScene();

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
Entity SpawnBox(const std::string& name, float x, float y, float width, float height, float density = 1.0f);

/**
 * @brief Spawns a static box entity.
 * @param name Entity name.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param width Box width.
 * @param height Box height.
 * @return Spawned Entity.
 */
Entity SpawnStaticBox(const std::string& name, float x, float y, float width, float height);

/**
 * @brief Spawns a dynamic circle entity.
 * @param name Entity name.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param radius Circle radius.
 * @param density Mass density.
 * @return Spawned Entity.
 */
Entity SpawnCircle(const std::string& name, float x, float y, float radius, float density = 1.0f);

/**
 * @brief Spawns a static circle entity.
 * @param name Entity name.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param radius Circle radius.
 * @return Spawned Entity.
 */
Entity SpawnStaticCircle(const std::string& name, float x, float y, float radius);

/**
 * @brief Finds an entity by name.
 * @param name Entity name.
 * @return Entity handle.
 */
Entity FindEntity(const std::string& name);

/**
 * @brief Gets all active entities in the scene.
 * @return Vector of Entity handles.
 */
std::vector<Entity> GetAllEntities();

/**
 * @brief Sets world gravity.
 * @param gx Gravity X component.
 * @param gy Gravity Y component.
 */
void SetGravity(float gx, float gy);

/**
 * @brief Sets world gravity.
 * @param gravity Gravity vector.
 */
void SetGravity(const Vector2& gravity);

/**
 * @brief Sets outer boundary limits.
 * @param centerX Boundary center X.
 * @param centerY Boundary center Y.
 * @param halfWidth Boundary half-width.
 * @param halfHeight Boundary half-height.
 */
void SetWorldBounds(float centerX, float centerY, float halfWidth, float halfHeight);

} // namespace Elysium
