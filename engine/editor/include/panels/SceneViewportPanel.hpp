#ifndef SCENEVIEWPORTPANEL_HPP
#define SCENEVIEWPORTPANEL_HPP

#include "EditorPanel.hpp"
#include <SFML/Graphics/RenderTexture.hpp>

namespace Elysium {

class SceneViewportPanel : public EditorPanel {
public:
  SceneViewportPanel();
  ~SceneViewportPanel() override = default;

  void OnImGuiRender(EditorContext &context, Scene &scene,
                     SimpleRenderer &renderer) override;

private:
  sf::RenderTexture m_viewportTexture;
  sf::Vector2u m_currentTextureSize = {0, 0};
};

} // namespace Elysium

#endif // SCENEVIEWPORTPANEL_HPP
