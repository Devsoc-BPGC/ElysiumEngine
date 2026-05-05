#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <memory>

class GameObject;

class Component {
public:
    GameObject* gameObject;
    
    virtual ~Component() = default;
    virtual void Start() {}
    virtual void Update(float dt) {}
};

#endif
