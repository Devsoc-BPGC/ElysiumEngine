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

  // Brings the panel to the front on the next frame, selecting its tab if
  // docked
  void RequestFocus() { m_focusRequested = true; }

private:
  sf::RenderTexture m_viewportTexture;
  sf::Vector2u m_currentTextureSize = {0, 0};
  bool m_focusRequested = false;
};

} // namespace Elysium

#endif // SCENEVIEWPORTPANEL_HPP
