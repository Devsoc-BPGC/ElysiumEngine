#ifndef TOOLBARPANEL_HPP
#define TOOLBARPANEL_HPP

#include "EditorPanel.hpp"

namespace Elysium {

class ToolbarPanel : public EditorPanel {
public:
  ToolbarPanel();
  ~ToolbarPanel() override = default;

  void OnImGuiRender(EditorContext &context, Scene &scene,
                     SimpleRenderer &renderer) override;
};

} // namespace Elysium

#endif // TOOLBARPANEL_HPP
