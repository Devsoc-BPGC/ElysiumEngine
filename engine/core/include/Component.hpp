#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <memory>

class GameObject;

class Component {
public:
  GameObject *gameObject;

  virtual ~Component() = default;
  virtual void Start() {}
  virtual void Update(float dt) {}

  // Returns a copy of this component, used when Play mode copies the scene.
  // The caller points the copy's gameObject at its new owner.
  virtual std::unique_ptr<Component> Clone() const = 0;
};

#endif
