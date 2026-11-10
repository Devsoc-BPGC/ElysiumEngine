#include "CameraComponent.hpp"

namespace Elysium {

std::unique_ptr<Component> CameraComponent::Clone() const {
  return std::make_unique<CameraComponent>(*this);
}

} // namespace Elysium
