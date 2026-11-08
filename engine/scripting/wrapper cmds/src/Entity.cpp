#include "Entity.hpp"
#include "GameObject.hpp"
#include "RigidBody.hpp"
#include "SpriteRenderer.hpp"
#include "Log.h"
#include <cmath>

namespace Elysium {

/**
 * @brief Constructs an Entity handle around a GameObject.
 * @param object Shared pointer to GameObject.
 */
Entity::Entity(std::shared_ptr<GameObject> object)
    : m_Object(object) {}

/**
 * @brief Checks whether the entity is valid and active.
 * @return true if valid, false otherwise.
 */
bool Entity::IsValid() const {
    auto obj = m_Object.lock();
    return obj && obj->active;
}

/**
 * @brief Marks the entity for destruction.
 */
void Entity::Destroy() {
    if (auto obj = m_Object.lock()) {
        obj->active = false;
    }
}

/**
 * @brief Gets the name of the entity.
 * @return Entity name.
 */
std::string Entity::GetName() const {
    if (auto obj = m_Object.lock()) {
        return obj->name;
    }
    return "";
}

/**
 * @brief Sets the world position.
 * @param x X coordinate.
 * @param y Y coordinate.
 */
void Entity::SetPosition(float x, float y) {
    if (auto obj = m_Object.lock()) {
        obj->position.x = x;
        obj->position.y = y;
        obj->SyncTransform();
    }
}

/**
 * @brief Sets the world position.
 * @param pos Position vector.
 */
void Entity::SetPosition(const Vector2& pos) {
    SetPosition(pos.x, pos.y);
}

/**
 * @brief Moves the entity by an offset.
 * @param dx Offset along X axis.
 * @param dy Offset along Y axis.
 */
void Entity::Move(float dx, float dy) {
    if (auto obj = m_Object.lock()) {
        obj->position.x += dx;
        obj->position.y += dy;
        obj->SyncTransform();
    }
}

/**
 * @brief Moves the entity by an offset.
 * @param delta Offset vector.
 */
void Entity::Move(const Vector2& delta) {
    Move(delta.x, delta.y);
}

/**
 * @brief Gets the world position.
 * @return Position as Vector2.
 */
Vector2 Entity::GetPosition() const {
    if (auto obj = m_Object.lock()) {
        return {obj->position.x, obj->position.y};
    }
    return {0.0f, 0.0f};
}

/**
 * @brief Gets the X coordinate.
 * @return X coordinate.
 */
float Entity::GetX() const {
    return GetPosition().x;
}

/**
 * @brief Gets the Y coordinate.
 * @return Y coordinate.
 */
float Entity::GetY() const {
    return GetPosition().y;
}

/**
 * @brief Sets rotation in radians.
 * @param angleRadians Angle in radians.
 */
void Entity::SetRotation(float angleRadians) {
    if (auto obj = m_Object.lock()) {
        obj->rotation = Quat(Vec3(0.0f, 0.0f, 1.0f), angleRadians);
        obj->SyncTransform();
    }
}

/**
 * @brief Gets rotation in radians.
 * @return Angle in radians.
 */
float Entity::GetRotation() const {
    if (auto obj = m_Object.lock()) {
        return 2.0f * std::atan2(obj->rotation.z, obj->rotation.w);
    }
    return 0.0f;
}

/**
 * @brief Sets the linear velocity.
 * @param vx Velocity along X.
 * @param vy Velocity along Y.
 */
void Entity::SetVelocity(float vx, float vy) {
    if (auto obj = m_Object.lock()) {
        if (obj->rigidBody) {
            obj->rigidBody->linearVelocity.x = vx;
            obj->rigidBody->linearVelocity.y = vy;
        }
    }
}

/**
 * @brief Sets the linear velocity.
 * @param vel Velocity vector.
 */
void Entity::SetVelocity(const Vector2& vel) {
    SetVelocity(vel.x, vel.y);
}

/**
 * @brief Sets the horizontal velocity.
 * @param vx Velocity along X.
 */
void Entity::SetVelocityX(float vx) {
    if (auto obj = m_Object.lock()) {
        if (obj->rigidBody) {
            obj->rigidBody->linearVelocity.x = vx;
        }
    }
}

/**
 * @brief Sets the vertical velocity.
 * @param vy Velocity along Y.
 */
void Entity::SetVelocityY(float vy) {
    if (auto obj = m_Object.lock()) {
        if (obj->rigidBody) {
            obj->rigidBody->linearVelocity.y = vy;
        }
    }
}

/**
 * @brief Gets the linear velocity.
 * @return Velocity as Vector2.
 */
Vector2 Entity::GetVelocity() const {
    if (auto obj = m_Object.lock()) {
        if (obj->rigidBody) {
            return {obj->rigidBody->linearVelocity.x, obj->rigidBody->linearVelocity.y};
        }
    }
    return {0.0f, 0.0f};
}

/**
 * @brief Applies a force at the center of mass.
 * @param fx Force along X.
 * @param fy Force along Y.
 */
void Entity::ApplyForce(float fx, float fy) {
    if (auto obj = m_Object.lock()) {
        if (obj->rigidBody) {
            obj->rigidBody->forceAccumulator.x += fx;
            obj->rigidBody->forceAccumulator.y += fy;
        }
    }
}

/**
 * @brief Applies a force at the center of mass.
 * @param force Force vector.
 */
void Entity::ApplyForce(const Vector2& force) {
    ApplyForce(force.x, force.y);
}

/**
 * @brief Applies a linear impulse.
 * @param ix Impulse along X.
 * @param iy Impulse along Y.
 */
void Entity::ApplyImpulse(float ix, float iy) {
    if (auto obj = m_Object.lock()) {
        if (obj->rigidBody && obj->rigidBody->inverseMass > 0.0f) {
            obj->rigidBody->linearVelocity.x += ix * obj->rigidBody->inverseMass;
            obj->rigidBody->linearVelocity.y += iy * obj->rigidBody->inverseMass;
        }
    }
}

/**
 * @brief Applies a linear impulse.
 * @param impulse Impulse vector.
 */
void Entity::ApplyImpulse(const Vector2& impulse) {
    ApplyImpulse(impulse.x, impulse.y);
}

/**
 * @brief Sets the restitution (bounciness).
 * @param restitution Restitution coefficient (0.0 to 1.0).
 */
void Entity::SetBounciness(float restitution) {
    if (auto obj = m_Object.lock()) {
        if (obj->rigidBody) {
            obj->rigidBody->restitution = restitution;
        }
    }
}

/**
 * @brief Sets the friction coefficient.
 * @param friction Friction coefficient.
 */
void Entity::SetFriction(float friction) {
    if (auto obj = m_Object.lock()) {
        if (obj->rigidBody) {
            obj->rigidBody->friction = friction;
        }
    }
}

/**
 * @brief Sets the mass.
 * @param mass Mass in kilograms.
 */
void Entity::SetMass(float mass) {
    if (auto obj = m_Object.lock()) {
        if (obj->rigidBody) {
            obj->rigidBody->mass = mass;
            obj->rigidBody->inverseMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
            if (mass <= 0.0f) {
                obj->rigidBody->isStatic = true;
            }
        }
    }
}

/**
 * @brief Gets the mass.
 * @return Mass in kilograms.
 */
float Entity::GetMass() const {
    if (auto obj = m_Object.lock()) {
        if (obj->rigidBody) {
            return obj->rigidBody->mass;
        }
    }
    return 0.0f;
}

/**
 * @brief Sets whether the entity is static.
 * @param isStatic True if static, false if dynamic.
 */
void Entity::SetStatic(bool isStatic) {
    if (auto obj = m_Object.lock()) {
        if (obj->rigidBody) {
            obj->rigidBody->isStatic = isStatic;
            if (isStatic) {
                obj->rigidBody->inverseMass = 0.0f;
                obj->rigidBody->linearVelocity = Vec3(0.0f, 0.0f, 0.0f);
            } else if (obj->rigidBody->mass > 0.0f) {
                obj->rigidBody->inverseMass = 1.0f / obj->rigidBody->mass;
            }
        }
    }
}

/**
 * @brief Sets the color tint.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
void Entity::SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (auto obj = m_Object.lock()) {
        for (auto& comp : obj->components) {
            if (auto* sprite = dynamic_cast<SpriteRenderer*>(comp.get())) {
                sprite->color = sf::Color(r, g, b, a);
                return;
            }
        }
    }
}

/**
 * @brief Sets the color tint.
 * @param color ColorRGBA value.
 */
void Entity::SetColor(ColorRGBA color) {
    SetColor(color.r, color.green, color.b, color.a);
}

} // namespace Elysium
