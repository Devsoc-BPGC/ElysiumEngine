#include "physics/TwoDimensionPhysics.hpp"

// Constructor
TwoDimensionalParticle::TwoDimensionalParticle(double x_pos, double y_pos, double mass) 
    : x(x_pos, mass), y(y_pos, mass) {
}

// Physics
void TwoDimensionalParticle::update(const Force& f) {
    x.update(f.x);
    y.update(f.y);
}

// Getters
double TwoDimensionalParticle::getX() const {
    return x.getPosition();
}

double TwoDimensionalParticle::getY() const {
    return y.getPosition();
}

double TwoDimensionalParticle::getMass() const {
    return x.getMass();
}

// Setters
void TwoDimensionalParticle::setPosition(double px, double py) {
    x.setPosition(px);
    y.setPosition(py);
}