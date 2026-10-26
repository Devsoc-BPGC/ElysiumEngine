#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <memory>

class GameObject;

class Component {
public:
  GameObject *gameObject = nullptr;

  virtual ~Component() = default;
  virtual void Start() {}
  virtual void Update(float dt) { (void)dt; }

  // Returns a copy of this component, used when Play mode copies the scene.
  // GameObject::Clone() points the copy's gameObject at its new owner, then
  // calls OnCloned() so it can rebind anything else tied to the original.
  virtual std::unique_ptr<Component> Clone() const = 0;
  virtual void OnCloned(const std::shared_ptr<GameObject> &owner) {
    (void)owner;
  }
};

namespace Elysium {
using Component = ::Component;
}

#endif
