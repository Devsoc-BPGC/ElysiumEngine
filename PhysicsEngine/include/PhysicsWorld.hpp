#include <vector>
#include "./CoreMath.hpp"
#include "./RigidBody.hpp"

class PhysicsWorld {
private:
    std::vector<RigidBody*> rigidBodies;
    
    Vec3 gravity = Vec3(0, -9.81f, 0);

public:
    PhysicsWorld() {}
    
    ~PhysicsWorld() {
        for (auto body : rigidBodies) {
            delete body;
        }
        rigidBodies.clear();
    }

    void AddBody(RigidBody* body) {
        rigidBodies.push_back(body);
    }

    void Step(float dt);
};
