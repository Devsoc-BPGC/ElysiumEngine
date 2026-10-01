# Elysium Engine — Editor Subsystem

The `engine/editor` module provides a dockable, modular game engine editor built with **Dear ImGui** (docking branch) and **SFML 3 / OpenGL**.

---

## Directory Contents

| Header | Source | Description |
|---|---|---|
| [`EditorLayer.hpp`](include/EditorLayer.hpp) | [`EditorLayer.cpp`](src/EditorLayer.cpp) | Main engine layer managing ImGui-SFML lifecycle, DockSpace, menu bar, and global shortcuts. |
| [`EditorContext.hpp`](include/EditorContext.hpp) | — | Shared editor state (play state, active selection, editor camera, performance metrics). |
| [`EditorPanel.hpp`](include/EditorPanel.hpp) | — | Abstract interface for modular panels (`OnImGuiRender(context, scene, renderer)`). |
| [`EditorLogSink.hpp`](include/EditorLogSink.hpp) | [`EditorLogSink.cpp`](src/EditorLogSink.cpp) | Thread-safe circular buffer sink hooked into `spdlog` for the Console panel. |
| **Panels** | | |
| [`SceneViewportPanel.hpp`](include/panels/SceneViewportPanel.hpp) | [`SceneViewportPanel.cpp`](src/panels/SceneViewportPanel.cpp) | Off-screen `sf::RenderTexture` viewport with independent pan & zoom camera and HUD overlay. |
| [`SceneHierarchyPanel.hpp`](include/panels/SceneHierarchyPanel.hpp) | [`SceneHierarchyPanel.cpp`](src/panels/SceneHierarchyPanel.cpp) | Entity tree listing with real-time text filter, selection highlight, and Add/Delete actions. |
| [`EntityInspectorPanel.hpp`](include/panels/EntityInspectorPanel.hpp) | [`EntityInspectorPanel.cpp`](src/panels/EntityInspectorPanel.cpp) | Component reflection for Transform, RigidBody2D, Collider2D, and SpriteRenderer. |
| [`ToolbarPanel.hpp`](include/panels/ToolbarPanel.hpp) | [`ToolbarPanel.cpp`](src/panels/ToolbarPanel.cpp) | Play, Pause, and Single-Frame Step simulation buttons with mode badges. |
| [`ConsolePanel.hpp`](include/panels/ConsolePanel.hpp) | [`ConsolePanel.cpp`](src/panels/ConsolePanel.cpp) | In-editor log viewer with level filtering (Trace, Info, Warn, Error) and search. |
| [`StatsPanel.hpp`](include/panels/StatsPanel.hpp) | [`StatsPanel.cpp`](src/panels/StatsPanel.cpp) | Real-time FPS sparkline plot, frame timing breakdown, entity count, and draw call stats. |

---

## Architecture

### ImGui Docking & Workspace
`EditorLayer` hosts a full-window DockSpace (`ImGui::DockSpaceOverViewport()`). Panels can be freely dragged, docked into tabs, split horizontally or vertically, or floated into separate windows. Panel visibility is toggled via the **View** menu.

### Decoupled Viewport Rendering
1. `SceneViewportPanel` queries the available content region in ImGui.
2. It resizes an internal `sf::RenderTexture` dynamically.
3. It constructs an `sf::View` based on the camera center position and zoom level.
4. `SimpleRenderer::Render()` draws the scene into the render texture.
5. The texture is displayed in ImGui using `ImGui::Image()`.
6. Mouse drag and scroll events inside the viewport update the camera independently of the editor UI.

### Component Reflection & Mutability
The `EntityInspectorPanel` reflects components attached to `context.selectedEntity`:
- **Transform**: Position, Rotation (degrees around Z), Scale. Edits immediately invoke `entity->SyncTransform()`.
- **RigidBody2D**: Static/Dynamic toggle, Mass, Linear Velocity, Angular Velocity, Friction, Restitution.
- **Colliders**: Displays attached Sphere colliders (radius) and Box colliders (half-extents).
- **SpriteRenderer**: Shape selection, Color picker (`ImGui::ColorEdit4`), and dimensions.
- **Add Component**: Contextual popup allowing new components to be attached at runtime.

---

## Adding a New Editor Panel

To create a new tool or diagnostic panel:

```cpp
#include "EditorPanel.hpp"
#include <imgui.h>

namespace Elysium {

class ProfilerDetailPanel : public EditorPanel {
public:
  ProfilerDetailPanel() : EditorPanel("Profiler Details") {}

  void OnImGuiRender(EditorContext& context, Scene& scene, SimpleRenderer& renderer) override {
    if (ImGui::Begin(name.c_str(), &isOpen)) {
      ImGui::Text("FPS: %.1f", context.stats.fps);
      ImGui::Text("Update Time: %.2f ms", context.stats.updateTimeMs);
    }
    ImGui::End();
  }
};

} // namespace Elysium
```
Register the panel in `EditorLayer`: `AddPanel<ProfilerDetailPanel>();`. It will automatically appear in the workspace and in the **View** menu.
