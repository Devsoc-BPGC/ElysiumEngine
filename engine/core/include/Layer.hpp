#ifndef LAYER_HPP
#define LAYER_HPP

#include "Core.hpp"
#include "Event.hpp"
#include "Timestep.hpp"
#include <string>

namespace Elysium {

class ELYSIUM_API Layer {
public:
  Layer(const std::string &name = "Layer");
  virtual ~Layer() = default;

  virtual void OnAttach() {}
  virtual void OnDetach() {}
  virtual void OnUpdate(Timestep ts) { (void)ts; }
  virtual void OnImGuiRender() {}
  virtual void OnEvent(Event &event) { (void)event; }

  const std::string &GetName() const { return m_DebugName; }

protected:
  std::string m_DebugName;
};

} // namespace Elysium

#endif // LAYER_HPP
