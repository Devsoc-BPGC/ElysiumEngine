#ifndef COLLISION_PAIR_HPP
#define COLLISION_PAIR_HPP

#include "RigidBody.hpp"

/**
 * @struct CollisionPair
 * @brief Represents a pair of rigid bodies that are potentially colliding.
 */
struct CollisionPair {
    RigidBody* A;
    RigidBody* B;

    /**
     * @brief Compares two CollisionPairs for equality.
     * * Two CollisionPairs are considered equal if they contain the same two rigid bodies,
     * regardless of the order (A, B) or (B, A).
     * * @param other The other CollisionPair to compare with.
     * @return true if the pairs are equal, false otherwise.
     */
    bool operator==(const CollisionPair& other) const {
        return (A == other.A && B == other.B) || (A == other.B && B == other.A);
    }
};

#endif
