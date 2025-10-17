#include "physics/Force.hpp"

// Constructor implementation
Force::Force(double x, double y) : x(x), y(y) {
}

// Getter implementations
double Force::getX() const {
    return x;
}

double Force::getY() const {
    return y;
}