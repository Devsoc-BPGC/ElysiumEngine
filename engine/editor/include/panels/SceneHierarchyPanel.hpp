#ifndef SCENEHIERARCHYPANEL_HPP
#define SCENEHIERARCHYPANEL_HPP

#include "EditorPanel.hpp"
#include <imgui.h>

namespace Elysium {

class SceneHierarchyPanel : public EditorPanel {
public:
  SceneHierarchyPanel();
  ~SceneHierarchyPanel() override = default;

  void OnImGuiRender(EditorContext &context, Scene &scene,
                     SimpleRenderer &renderer) override;

private:
  void DrawEntityNode(EditorContext &context, Scene &scene,
                      const std::shared_ptr<GameObject> &obj,
                      std::shared_ptr<GameObject> &entityToDelete);

  ImGuiTextFilter m_filter;
};

} // namespace Elysium

#endif // SCENEHIERARCHYPANEL_HPP
