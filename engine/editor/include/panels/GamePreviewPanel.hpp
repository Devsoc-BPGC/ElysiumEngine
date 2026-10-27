#ifndef GAMEPREVIEWPANEL_HPP
#define GAMEPREVIEWPANEL_HPP

#include "EditorPanel.hpp"
#include <SFML/Graphics/RenderTexture.hpp>

namespace Elysium {

// Shows the scene as the player sees it. Unlike SceneViewportPanel it is
// driven by EditorContext::gameCamera, keeps a fixed output aspect ratio
// (letterboxed), hides editor-only visuals unless explicitly enabled, and
// while playing shows the simulated copy of the scene.
class GamePreviewPanel : public EditorPanel {
public:
  GamePreviewPanel();
  ~GamePreviewPanel() override = default;

  void OnImGuiRender(EditorContext &context, Scene &scene,
                     SimpleRenderer &renderer) override;

  // Brings the panel to the front on the next frame, expanding it and
  // selecting its tab if docked
  void RequestFocus() { m_focusRequested = true; }

private:
  bool EnsureTarget(unsigned int width, unsigned int height);

  sf::RenderTexture m_previewTexture;
  sf::Vector2u m_currentTextureSize = {0, 0};
  bool m_focusRequested = false;

  // The window is only created once the scene viewport's geometry has settled
  bool m_windowCreated = false;
  sf::Vector2f m_lastViewportPos = {-1.0f, -1.0f};
  sf::Vector2f m_lastViewportSize = {-1.0f, -1.0f};
};

} // namespace Elysium

#endif // GAMEPREVIEWPANEL_HPP
