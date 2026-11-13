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
  void DrawAudioSourceComponent(EditorContext &context, GameObject &entity);
  void DrawCameraComponent(EditorContext &context, Scene &scene, GameObject &entity);
  void DrawCustomComponents(EditorContext &context, GameObject &entity);

  // Tracking for undo/redo coalescing during continuous inspector dragging
  Vec3 m_dragStartPos{0.0f, 0.0f, 0.0f};
  Quat m_dragStartRot;
  Vec3 m_dragStartScale{1.0f, 1.0f, 1.0f};
  float m_dragStartFloat = 0.0f;
  Vec3 m_dragStartVec3{0.0f, 0.0f, 0.0f};
  std::string m_dragStartString;
  bool m_dragStartBool = false;
};

} // namespace Elysium

#endif // ENTITYINSPECTORPANEL_HPP
