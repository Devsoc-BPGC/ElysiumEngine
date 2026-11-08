#pragma once

#include "Core.hpp"
#include "ScriptTypes.hpp"
#include "Script.hpp"
#include "GameObject.hpp"
#include <memory>
#include <string>
#include <type_traits>
#include <stdexcept>

namespace Elysium {

/**
 * @class Entity
 * @brief Handle representing a GameObject in the scene.
 */
class ELYSIUM_API Entity {
public:
    Entity() = default;
    Entity(std::shared_ptr<GameObject> object);

    /**
     * @brief Checks if the entity is valid and active in the scene.
     * @return true if valid, false otherwise.
     */
    bool IsValid() const;

    /**
     * @brief Marks the entity for destruction.
     */
    void Destroy();

    /**
     * @brief Gets the name of the entity.
     * @return Entity name.
     */
    std::string GetName() const;

    // --- Transform ---

    /**
     * @brief Sets the world position.
     * @param x X coordinate.
     * @param y Y coordinate.
     */
    void SetPosition(float x, float y);

    /**
     * @brief Sets the world position.
     * @param pos Position vector.
     */
    void SetPosition(const Vector2& pos);

    /**
     * @brief Moves the entity by an offset.
     * @param dx Offset along X axis.
     * @param dy Offset along Y axis.
     */
    void Move(float dx, float dy);

    /**
     * @brief Moves the entity by an offset.
     * @param delta Offset vector.
     */
    void Move(const Vector2& delta);

    /**
     * @brief Gets the world position.
     * @return Position as Vector2.
     */
    Vector2 GetPosition() const;

    /**
     * @brief Gets the X coordinate.
     * @return X coordinate.
     */
    float GetX() const;

    /**
     * @brief Gets the Y coordinate.
     * @return Y coordinate.
     */
    float GetY() const;

    /**
     * @brief Sets rotation in radians.
     * @param angleRadians Angle in radians.
     */
    void SetRotation(float angleRadians);

    /**
     * @brief Gets rotation in radians.
     * @return Angle in radians.
     */
    float GetRotation() const;

    // --- Physics ---

    /**
     * @brief Sets the linear velocity.
     * @param vx Velocity along X.
     * @param vy Velocity along Y.
     */
    void SetVelocity(float vx, float vy);

    /**
     * @brief Sets the linear velocity.
     * @param vel Velocity vector.
     */
    void SetVelocity(const Vector2& vel);

    /**
     * @brief Sets the horizontal velocity.
     * @param vx Velocity along X.
     */
    void SetVelocityX(float vx);

    /**
     * @brief Sets the vertical velocity.
     * @param vy Velocity along Y.
     */
    void SetVelocityY(float vy);

    /**
     * @brief Gets the linear velocity.
     * @return Velocity as Vector2.
     */
    Vector2 GetVelocity() const;

    /**
     * @brief Applies a force at the center of mass.
     * @param fx Force along X.
     * @param fy Force along Y.
     */
    void ApplyForce(float fx, float fy);

    /**
     * @brief Applies a force at the center of mass.
     * @param force Force vector.
     */
    void ApplyForce(const Vector2& force);

    /**
     * @brief Applies a linear impulse.
     * @param ix Impulse along X.
     * @param iy Impulse along Y.
     */
    void ApplyImpulse(float ix, float iy);

    /**
     * @brief Applies a linear impulse.
     * @param impulse Impulse vector.
     */
    void ApplyImpulse(const Vector2& impulse);

    /**
     * @brief Sets the restitution (bounciness).
     * @param restitution Restitution coefficient (0.0 to 1.0).
     */
    void SetBounciness(float restitution);

    /**
     * @brief Sets the friction coefficient.
     * @param friction Friction coefficient.
     */
    void SetFriction(float friction);

    /**
     * @brief Sets the mass.
     * @param mass Mass in kilograms.
     */
    void SetMass(float mass);

    /**
     * @brief Gets the mass.
     * @return Mass in kilograms.
     */
    float GetMass() const;

    /**
     * @brief Sets whether the entity is static.
     * @param isStatic True if static, false if dynamic.
     */
    void SetStatic(bool isStatic);

    // --- Rendering ---

    /**
     * @brief Sets the color tint.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param a Alpha component (0-255).
     */
    void SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    /**
     * @brief Sets the color tint.
     * @param color ColorRGBA value.
     */
    void SetColor(ColorRGBA color);

    // --- Scripts ---

    /**
     * @brief Attaches a script to the entity.
     * @tparam T Script type derived from Script.
     * @param args Constructor arguments for T.
     * @return Reference to the attached script.
     */
    template<typename T, typename... Args>
    T& AttachScript(Args&&... args) {
        static_assert(std::is_base_of<Script, T>::value, "T must inherit from Elysium::Script");
        auto obj = m_Object.lock();
        if (!obj) {
            throw std::runtime_error("Cannot attach script to invalid Entity");
        }
        T& script = obj->AddComponent<T>(std::forward<Args>(args)...);
        script.m_EntityHolder = std::make_shared<Entity>(obj);
        script.entity = script.m_EntityHolder.get();
        script.OnCreate();
        return script;
    }

    /**
     * @brief Retrieves an attached script of type T.
     * @tparam T Script type.
     * @return Pointer to script if found, nullptr otherwise.
     */
    template<typename T>
    T* GetScript() {
        auto obj = m_Object.lock();
        if (!obj) return nullptr;
        for (auto& comp : obj->components) {
            if (auto* casted = dynamic_cast<T*>(comp.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    /**
     * @brief Gets the underlying GameObject pointer.
     * @return Shared pointer to GameObject.
     */
    std::shared_ptr<GameObject> GetInternalObject() const { return m_Object.lock(); }

private:
    std::weak_ptr<GameObject> m_Object;
};

} // namespace Elysium
