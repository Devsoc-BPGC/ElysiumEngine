#include "physics/OneDimensionPhysics.hpp"
#include <cassert>

// Define the constant
const double DELTA_T = 0.01;

// Constructor implementation
OneDimensionalParticle::OneDimensionalParticle(double pos, double mass) 
    : pos(pos), prevPos(pos), mass(mass) {
    assert(mass != 0);
}

// Getter implementations
double OneDimensionalParticle::getPosition() const {
    return pos;
}

double OneDimensionalParticle::getMass() const {
    return mass;
}

// Setter implementation
void OneDimensionalParticle::setPosition(double p) {
    pos = p;
    prevPos = p;
}

// Physics update implementation
void OneDimensionalParticle::update(double force) {
    double accel = force / mass;
    double temp = pos;
    pos = 2.0 * pos - prevPos + DELTA_T * DELTA_T * accel;
    prevPos = temp;
}