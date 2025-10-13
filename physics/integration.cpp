#pragma once // i think im supposed to use this
#include <cassert>

class oneDimensionalParticle
{
private:
    double pos, mass, vel; // oneDimensionalParticle co-ordinates and shi
public:

    oneDimensionalParticle(double pos, double mass, double vel): pos(pos), mass(mass), vel(vel){
        assert(mass!=0);
    } // Constructor

    
};

int main() {
    oneDimensionalParticle* particle = new oneDimensionalParticle(0,10,0); // hard coding values for now
    

}