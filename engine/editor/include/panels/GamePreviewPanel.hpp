#ifndef GAMEPREVIEWPANEL_HPP
#define GAMEPREVIEWPANEL_HPP

#include "EditorPanel.hpp"
#include <SFML/Graphics/RenderTexture.hpp>

namespace Elysium {

// Shows the scene as the player sees it. Unlike SceneViewportPanel it is
// driven by EditorContext::gameCamera, keeps a fixed output aspect ratio
// (letterboxed), and hides editor-only visuals unless explicitly enabled.
class GamePreviewPanel : public EditorPanel {
public:
  GamePreviewPanel();
  ~GamePreviewPanel() override = default;

  void OnImGuiRender(EditorContext &context, Scene &scene,
                     SimpleRenderer &renderer) override;

private:
  bool EnsureTarget(unsigned int width, unsigned int height);

  sf::RenderTexture m_previewTexture;
  sf::Vector2u m_currentTextureSize = {0, 0};
};

} // namespace Elysium

#endif // GAMEPREVIEWPANEL_HPP
