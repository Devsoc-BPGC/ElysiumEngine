#ifndef TWO_DIMENSION_PHYSICS_HPP
#define TWO_DIMENSION_PHYSICS_HPP

#include "physics/Force.hpp"
#include "physics/OneDimensionPhysics.hpp"

class TwoDimensionalParticle
{
private:
    OneDimensionalParticle x;
    OneDimensionalParticle y;

public:
    // Constructor
    TwoDimensionalParticle(double x_pos, double y_pos, double mass);
    
    // Destructor (default is fine with stack allocation)
    ~TwoDimensionalParticle() = default;

    // Update with force
    void update(const Force& f);
    
    // Getters
    double getX() const;
    double getY() const;
    double getMass() const;
    
    // Setters
    void setPosition(double px, double py);
};

#endif // TWO_DIMENSION_PHYSICS_HPP
