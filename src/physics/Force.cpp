#include "physics/Force.hpp"

// Constructor
Force::Force(double x, double y) : x(x), y(y) {
}

// Getters
double Force::getX() const {
    return x;
}

double Force::getY() const {
    return y;
}