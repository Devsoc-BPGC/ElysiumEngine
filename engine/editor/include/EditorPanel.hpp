#ifndef EDITORPANEL_HPP
#define EDITORPANEL_HPP

#include "EditorContext.hpp"
#include "Scene.hpp"
#include "SimpleRenderer.hpp"
#include <string>

namespace Elysium {

class EditorPanel {
public:
  virtual ~EditorPanel() = default;
  virtual void OnImGuiRender(EditorContext &context, Scene &scene,
                             SimpleRenderer &renderer) = 0;

  bool isOpen = true;
  std::string name;

  explicit EditorPanel(std::string panelName) : name(std::move(panelName)) {}
};

} // namespace Elysium

#endif // EDITORPANEL_HPP
