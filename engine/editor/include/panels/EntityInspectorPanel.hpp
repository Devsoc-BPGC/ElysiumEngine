#ifndef ENTITYINSPECTORPANEL_HPP
#define ENTITYINSPECTORPANEL_HPP

#include "EditorPanel.hpp"

namespace Elysium {

class EntityInspectorPanel : public EditorPanel {
public:
  EntityInspectorPanel();
  ~EntityInspectorPanel() override = default;

  void OnImGuiRender(EditorContext &context, Scene &scene,
                     SimpleRenderer &renderer) override;

private:
  void DrawTransformComponent(EditorContext &context, GameObject &entity);
  void DrawRigidBodyComponent(EditorContext &context, Scene &scene,
                              GameObject &entity);
  void DrawCollidersComponent(EditorContext &context, GameObject &entity);
  void DrawSpriteRendererComponent(EditorContext &context, GameObject &entity);
  void DrawCustomComponents(EditorContext &context, GameObject &entity);
};

} // namespace Elysium

#endif // ENTITYINSPECTORPANEL_HPP
