#ifndef ONE_DIMENSION_PHYSICS_HPP
#define ONE_DIMENSION_PHYSICS_HPP

// Declare the constant (defined in .cpp)
extern const double DELTA_T;

class OneDimensionalParticle
{
private:
    double pos, prevPos, mass;
    
public:
    // Constructor
    OneDimensionalParticle(double pos, double mass);
    
    // Getters
    double getPosition() const;
    double getMass() const;
    
    // Setters
    void setPosition(double p);
    
    // Physics update using verlet integration
    void update(double force);
};

#endif // ONE_DIMENSION_PHYSICS_HPP
