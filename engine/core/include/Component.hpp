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
};

namespace Elysium {
using Component = ::Component;
}

#endif
