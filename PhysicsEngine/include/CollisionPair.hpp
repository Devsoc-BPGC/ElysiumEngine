#ifndef COLLISION_PAIR_HPP
#define COLLISION_PAIR_HPP

#include "RigidBody.hpp"

struct CollisionPair {
    RigidBody* A;
    RigidBody* B;

    // Helper to ensure (A, B) is the same as (B, A) to avoid duplicates
    bool operator==(const CollisionPair& other) const {
        return (A == other.A && B == other.B) || (A == other.B && B == other.A);
    }
};

#endif
