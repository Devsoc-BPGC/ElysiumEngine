#ifndef SCENEVIEWPORTPANEL_HPP
#define SCENEVIEWPORTPANEL_HPP

#include "EditorPanel.hpp"
#include "TransformGizmo.hpp"
#include <SFML/Graphics/RenderTexture.hpp>

namespace Elysium {

class SceneViewportPanel : public EditorPanel {
public:
  SceneViewportPanel();
  ~SceneViewportPanel() override = default;

  void OnImGuiRender(EditorContext &context, Scene &scene,
                     SimpleRenderer &renderer) override;

  // Brings the panel to the front, selecting its tab if docked
  void RequestFocus() { m_focusRequested = true; }

  TransformGizmo &GetGizmo() { return m_gizmo; }

private:
  sf::RenderTexture m_viewportTexture;
  sf::Vector2u m_currentTextureSize = {0, 0};
  bool m_focusRequested = false;
  TransformGizmo m_gizmo;
};

} // namespace Elysium

#endif // SCENEVIEWPORTPANEL_HPP
