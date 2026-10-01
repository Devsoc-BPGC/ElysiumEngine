# Elysium Engine — Core Architecture

The `engine/core` module forms the foundational layer of the Elysium Game Engine. It manages the application lifecycle, platform windowing, the dynamic layer stack, game timing, input abstraction, and entity-component representations.

---

## Directory Contents

| Header | Source | Description |
|---|---|---|
| [`Application.hpp`](include/Application.hpp) | [`Application.cpp`](src/Application.cpp) | Central engine singleton driving the master loop, window ownership, and layer execution. |
| [`Window.hpp`](include/Window.hpp) | [`Window.cpp`](src/Window.cpp) | Encapsulates `sf::RenderWindow`, driving the SFML 3 event pump and event translation. |
| [`Layer.hpp`](include/Layer.hpp) | [`Layer.cpp`](src/Layer.cpp) | Abstract base interface for game layers and editor overlays. |
| [`LayerStack.hpp`](include/LayerStack.hpp) | [`LayerStack.cpp`](src/LayerStack.cpp) | Ordered container distinguishing normal layers from top-level overlays. |
| [`Timestep.hpp`](include/Timestep.hpp) | — | Frame delta time wrapper with implicit float conversion and millisecond helpers. |
| [`KeyCodes.hpp`](include/KeyCodes.hpp) | — | Framework-independent keyboard key definitions matching SFML 3 keys. |
| [`MouseCodes.hpp`](include/MouseCodes.hpp) | — | Framework-independent mouse button definitions matching SFML 3 buttons. |
| [`Input.hpp`](include/Input.hpp) | — | Static polling interface for held/pressed keyboard keys and mouse buttons. |
| [`GameObject.hpp`](include/GameObject.hpp) | — | Entity class managing transform data, attached RigidBody, and component collection. |
| [`Component.hpp`](include/Component.hpp) | — | Base class for user-defined components (`Start()`, `Update(dt)`). |
| [`SpriteRenderer.hpp`](include/SpriteRenderer.hpp) | — | Visual primitive component (box or circle shapes with tint color). |
| [`Scene.hpp`](include/Scene.hpp) | — | Scene graph holding entities and managing physics world stepping/synchronization. |
| [`SimpleRenderer.hpp`](include/SimpleRenderer.hpp) | — | Target-agnostic renderer drawing scene geometry to any `sf::RenderTarget` (e.g. `sf::RenderTexture`). |
| [`Log.h`](include/Log.h) | [`Log.cpp`](src/Log.cpp) | Core and Client logging macros powered by `spdlog`. |
| [`Core.hpp`](include/Core.hpp) | — | Platform detection macros and DLL export/import definitions. |
| [`EntryPoint.hpp`](include/EntryPoint.hpp) | — | Platform `main()` entry point calling `Elysium::CreateApplication()`. |
| [`Elysium.hpp`](include/Elysium.hpp) | — | Main umbrella header for client projects using the engine. |

---

## Key Concepts

### Application & Master Game Loop
The `Application` class maintains the engine state:
1. Constructing an instance initializes the `Window` and creates an event dispatch pipeline.
2. In `Run()`, the loop calculates frame delta time, calls `Window::OnUpdate()`, iterates through the `LayerStack` calling `OnUpdate(timestep)`, runs `OnImGuiRender()`, and swaps framebuffers.

### Layer Stack Architecture
- **Layers**: Standard layers (e.g., game world, gameplay systems) pushed via `PushLayer()`. Updated front-to-back, receiving events after overlays.
- **Overlays**: UI and diagnostic layers (e.g., `EditorLayer`) pushed via `PushOverlay()`. Always rendered last and receive events first.

### Transform Synchronization
When an entity has a `RigidBody`:
- Physics simulation updates write to the entity's position and rotation via `entity->SyncPhysics()`.
- Manual transform updates in the Editor Inspector synchronize the physics simulation via `entity->SyncTransform()`.
