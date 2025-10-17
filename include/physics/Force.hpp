#ifndef FORCE_HPP
#define FORCE_HPP

class Force
{
public:
    double x;
    double y;
    
    // Constructor
    Force(double x = 0.0, double y = 0.0);
    
    // Getters
    double getX() const;
    double getY() const;
};

#endif // FORCE_HPP
