#ifndef CAMERACOMPONENT_HPP
#define CAMERACOMPONENT_HPP

#include "Component.hpp"
#include "SceneCamera.hpp"
#include <SFML/Graphics/Color.hpp>

namespace Elysium {

/**
 * @brief Component that attaches a SceneCamera to a GameObject.
 */
class CameraComponent : public Component {
public:
  SceneCamera Camera;
  bool Primary = true;
  bool FixedAspectRatio = false;
  sf::Color ClearColor = sf::Color(119, 221, 119);

  CameraComponent() = default;
  explicit CameraComponent(bool primary, bool fixedAspect = false,
                           sf::Color clearCol = sf::Color(119, 221, 119))
      : Primary(primary), FixedAspectRatio(fixedAspect), ClearColor(clearCol) {}

  std::unique_ptr<Component> Clone() const override;
};

} // namespace Elysium

using CameraComponent = Elysium::CameraComponent;

#endif // CAMERACOMPONENT_HPP
