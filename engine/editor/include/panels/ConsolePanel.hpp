#ifndef CONSOLEPANEL_HPP
#define CONSOLEPANEL_HPP

#include "EditorPanel.hpp"
#include <imgui.h>

namespace Elysium {

class ConsolePanel : public EditorPanel {
public:
  ConsolePanel();
  ~ConsolePanel() override = default;

  void OnImGuiRender(EditorContext &context, Scene &scene,
                     SimpleRenderer &renderer) override;

private:
  ImGuiTextFilter m_filter;
  bool m_showTrace = true;
  bool m_showInfo = true;
  bool m_showWarn = true;
  bool m_showError = true;
  bool m_autoScroll = true;
};

} // namespace Elysium

#endif // CONSOLEPANEL_HPP
