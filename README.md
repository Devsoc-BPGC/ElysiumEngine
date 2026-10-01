# Elysium Engine

> A modular, high-performance 2D Game Engine and Editor built with **C++20**, **SFML 3.2**, and **Dear ImGui** (docking branch).

Elysium has evolved from a standalone 2D physics engine into a full-featured 2D Game Engine featuring a dockable editor suite, type-safe event architecture, decoupled layer/overlay hierarchy, offscreen viewport rendering, and interactive component reflection.

---

## Table of Contents

- [Overview & Architecture](#overview--architecture)
- [Key Features](#key-features)
- [Project Directory Layout](#project-directory-layout)
- [Subsystem Guides](#subsystem-guides)
  - [1. Application & Layer Lifecycle](#1-application--layer-lifecycle)
  - [2. Event System & Input Handling](#2-event-system--input-handling)
  - [3. Game Engine Editor & Docking UI](#3-game-engine-editor--docking-ui)
  - [4. Viewport & Rendering Pipeline](#4-viewport--rendering-pipeline)
  - [5. Scene, Entities & Components](#5-scene-entities--components)
  - [6. 2D Physics Simulation](#6-2d-physics-simulation)
- [Prerequisites & Dependencies](#prerequisites--dependencies)
- [Building & Running](#building--running)
- [Developer & Contributor Guide](#developer--contributor-guide)
  - [Creating a New Layer](#creating-a-new-layer)
  - [Creating a Custom Editor Panel](#creating-a-custom-editor-panel)
  - [Creating a Custom Component](#creating-a-custom-component)
- [Roadmap](#roadmap)
- [License](#license)

---

## Overview & Architecture

Elysium follows a modular, decoupled engine design:

```
+-------------------------------------------------------------------------+
|                              ElysiumApp                                 |
|                 (Client Entry Point / Sandbox Application)              |
+-------------------------------------------------------------------------+
                                     |
                                     v
+-------------------------------------------------------------------------+
|                            Application Core                             |
|  - Master Game Loop (Variable/Fixed Timestep via sf::Clock)             |
|  - LayerStack (Game Layers & Editor UI Overlays)                        |
|  - Window Abstraction (sf::RenderWindow + SFML 3 Event Pump)            |
+-------------------------------------------------------------------------+
            |                                            |
            v                                            v
+------------------------+                  +------------------------+
|      Event System      |                  |      Editor Layer      |
|  - Type-Safe Events    |                  |  - ImGui DockSpace     |
|  - EventDispatcher     |                  |  - Menu Bar & Hotkeys  |
|  - Key & Mouse Codes   |                  |  - Modular Panels      |
+------------------------+                  +------------------------+
            |                                            |
            v                                            v
+------------------------+                  +------------------------+
|    Scene & Entities    |                  |   Offscreen Viewport   |
|  - GameObject System   |                  |  - sf::RenderTexture   |
|  - Component Pipeline  |                  |  - 2D Editor Camera    |
|  - SpriteRenderer      |                  |  - SimpleRenderer      |
+------------------------+                  +------------------------+
                                     |
                                     v
+-------------------------------------------------------------------------+
|                             Physics Engine                              |
|  - RigidBody Dynamics & Inertia Tensors                                 |
|  - BroadPhase Spatial Grid & NarrowPhase SAT Detection                  |
|  - Circle & Box Colliders                                               |
+-------------------------------------------------------------------------+
```

---

## Key Features

- **Dockable Engine Editor**:
  - ImGui docking branch with workspace persistence and custom modern dark theme.
  - Comprehensive Main Menu Bar (`File`, `Edit`, `Scene`, `Entities`, `View`, `Options`).
  - Play, Pause, and Single-Frame Step toolbar controls to toggle between Edit Mode and Simulation Mode.
- **Off-Screen Viewport**:
  - Scene is rendered into an `sf::RenderTexture` and displayed via `ImGui::Image`.
  - Independent 2D Editor Camera with smooth panning (Right-Click / Middle-Click drag) and zooming (Mouse Wheel), completely decoupled from the editor UI.
- **Scene Hierarchy & Entity Inspector**:
  - Filterable live entity list with selection highlight.
  - Interactive component reflection for `Transform` (Position, Rotation, Scale), `RigidBody2D` (Mass, Linear/Angular Velocity, Static/Dynamic toggle, Friction, Restitution), `Collider2D` (Sphere radius, Box extents), and `SpriteRenderer`.
  - Instant two-way synchronization: inspector edits sync physics bodies and transforms in real time.
- **Application & LayerStack**:
  - `Application` singleton owns the master game loop, window, and `LayerStack`.
  - Distinguishes standard `Layer`s (gameplay, simulation) from `Overlay`s (editor, diagnostics).
- **Zero-Allocation Event System**:
  - Type-safe event hierarchy (`WindowCloseEvent`, `WindowResizeEvent`, `KeyPressedEvent`, `MouseButtonPressedEvent`, `MouseScrolledEvent`, etc.).
  - Templated `EventDispatcher` routing events with category bitmasks.
- **Real-Time Diagnostics**:
  - In-engine **Console Panel** hooked directly into `spdlog` via a custom ring-buffered `EditorLogSink`.
  - **Stats & Profiler Panel** showing real-time FPS graphs, frame timing breakdown (Update, Physics, Render), entity counts, and draw call statistics.
- **2D Physics Simulation**:
  - Continuous impulse-based rigid body dynamics, broadphase spatial hash grid, and narrowphase SAT collision response.

---

## Project Directory Layout

```text
ElysiumEngine/
├── CMakeLists.txt              # Root build configuration (FetchContent for ImGui & ImGui-SFML)
├── README.md                   # Main engine documentation
├── PLAN.md                     # Long-term development roadmap and technical design
├── LICENSE                     # MIT License
├── app/
│   └── main.cpp                # Client entry point (instantiates Sandbox Application)
├── engine/
│   ├── CMakeLists.txt          # Engine library build target (ElysiumEngine)
│   ├── core/                   # Application core, windowing, and entity systems
│   │   ├── include/
│   │   │   ├── Application.hpp     # Application singleton and master loop
│   │   │   ├── Window.hpp          # sf::RenderWindow encapsulation and event pump
│   │   │   ├── Layer.hpp           # Base class for engine layers
│   │   │   ├── LayerStack.hpp      # Ordered layer and overlay stack
│   │   │   ├── Timestep.hpp        # Frame delta time wrapper (seconds/ms)
│   │   │   ├── KeyCodes.hpp        # Platform-independent key codes
│   │   │   ├── MouseCodes.hpp      # Platform-independent mouse codes
│   │   │   ├── GameObject.hpp      # Entity definition and component container
│   │   │   ├── Component.hpp       # Component base interface
│   │   │   ├── SpriteRenderer.hpp  # Visual shape/sprite renderer component
│   │   │   ├── Scene.hpp           # Scene container and physics world binding
│   │   │   ├── SimpleRenderer.hpp  # Off-screen & target-agnostic 2D renderer
│   │   │   ├── Input.hpp           # Static keyboard and mouse polling
│   │   │   ├── Log.h               # spdlog logging macros
│   │   │   ├── Core.hpp            # Platform macros and DLL exports
│   │   │   ├── EntryPoint.hpp      # Platform main() bootstrap
│   │   │   └── Elysium.hpp         # Umbrella engine header for client apps
│   │   └── src/
│   │       ├── Application.cpp     # Engine game loop and event distribution
│   │       ├── Window.cpp          # SFML 3 event pump and translation
│   │       ├── Layer.cpp           # Layer base implementation
│   │       ├── LayerStack.cpp      # LayerStack management
│   │       └── Log.cpp             # spdlog initialization
│   ├── editor/                 # Dear ImGui Editor UI module
│   │   ├── include/
│   │   │   ├── EditorLayer.hpp     # Engine layer driving ImGui lifecycle & docking
│   │   │   ├── EditorContext.hpp   # Shared state (camera, selection, play state, stats)
│   │   │   ├── EditorPanel.hpp     # Abstract modular editor panel interface
│   │   │   ├── EditorLogSink.hpp   # spdlog memory sink for in-editor console
│   │   │   └── panels/             # Concrete panel definitions
│   │   │       ├── SceneViewportPanel.hpp    # Off-screen texture & pan/zoom camera
│   │   │       ├── SceneHierarchyPanel.hpp   # Entity list, search, add/delete
│   │   │       ├── EntityInspectorPanel.hpp  # Component reflection and mutation
│   │   │       ├── ToolbarPanel.hpp          # Play, Pause, Step controls
│   │   │       ├── ConsolePanel.hpp          # Filtered log viewer
│   │   │       └── StatsPanel.hpp            # Profiler, FPS plot, and timings
│   │   └── src/
│   │       ├── EditorLayer.cpp     # DockSpace, theme, menu bar, and shortcuts
│   │       ├── EditorLogSink.cpp   # Circular log buffer implementation
│   │       └── panels/             # Panel implementations
│   ├── events/                 # Event system architecture
│   │   ├── Event.hpp           # Base Event, EventDispatcher, categories
│   │   ├── ApplicationEvent.hpp# WindowCloseEvent, WindowResizeEvent
│   │   ├── KeyEvent.hpp        # KeyPressedEvent, KeyReleasedEvent, KeyTypedEvent
│   │   └── MouseEvent.hpp      # MouseMovedEvent, MouseScrolledEvent, MouseButtonEvent
│   └── physics/                # 2D Physics Engine
│       ├── include/
│       │   ├── RigidBody.hpp       # Mass, inertia, velocities, colliders
│       │   ├── Collider.hpp        # Sphere and Box collider geometry
│       │   ├── PhysicsWorld.hpp    # Simulation manager and solver loop
│       │   ├── BroadPhase.hpp      # Spatial hash grid
│       │   ├── NarrowPhase.hpp     # SAT collision detection and resolution
│       │   ├── AABB.hpp            # Axis-aligned bounding boxes
│       │   ├── CollisionPair.hpp   # Potential contact pair representation
│       │   └── CoreMath.hpp        # Vec2, Vec3, Mat3, Quat math primitives
│       └── src/
│           ├── AABB.cpp
│           ├── Collider.cpp
│           ├── PhysicsWorld.cpp
│           └── RigidBody.cpp
├── external/                   # Git submodules
│   ├── SFML/                   # SFML 3.2.0 source
│   └── spdlog/                 # spdlog header-only logging
└── test/                       # Unit tests and benchmarks
    ├── BoundaryTest.cpp
    ├── BroadPhasePerformanceTest.cpp
    ├── FrictionTest.cpp
    └── PhysicsPerformanceTest.cpp
```

---

## Subsystem Guides

### 1. Application & Layer Lifecycle

Client applications inherit from [`Elysium::Application`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/Application.hpp). The engine owns the window and controls the main loop:

```cpp
#include <Elysium.hpp>

class Sandbox : public Elysium::Application {
public:
  Sandbox() : Application("My Elysium Game") {
    // Push regular game layers:
    // PushLayer(new GameplayLayer());

    // Push the editor UI as an overlay (rendered on top):
    PushOverlay(new Elysium::EditorLayer());
  }

  ~Sandbox() override = default;
};

// Engine entry point defines the factory function:
Elysium::Application *Elysium::CreateApplication() {
  return new Sandbox();
}
```

The master loop in `Application::Run()` executes every frame:
1. Calculates frame delta time and constructs a [`Timestep`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/Timestep.hpp).
2. Calls `Window::OnUpdate()`, polling OS events and translating them into engine events.
3. Iterates front-to-back through `LayerStack`, calling `layer->OnUpdate(timestep)`.
4. Executes the ImGui rendering pass for all layers (`layer->OnImGuiRender()`).
5. Calls `Window::Display()` to swap frame buffers.

---

### 2. Event System & Input Handling

Events inherit from [`Event`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/events/Event.hpp) and carry category bitmasks:

```cpp
void MyLayer::OnEvent(Elysium::Event& event) {
  Elysium::EventDispatcher dispatcher(event);

  // Bind type-safe event handlers
  dispatcher.Dispatch<Elysium::KeyPressedEvent>([this](Elysium::KeyPressedEvent& e) {
    if (e.GetKeyCode() == Elysium::Key::Space) {
      ELYSIUM_INFO("Space key was pressed!");
      return true; // Mark as handled so layers below do not receive it
    }
    return false;
  });
}
```

Events are dispatched down the `LayerStack` in **reverse order**: Overlays (such as the Editor) receive events first. If an overlay handles the event (`event.Handled = true`), lower gameplay layers will not receive it.

---

### 3. Game Engine Editor & Docking UI

[`EditorLayer`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/editor/include/EditorLayer.hpp) is implemented as an engine `Overlay`. It manages:
- **DockSpace**: Full-screen docking host window (`ImGui::DockSpaceOverViewport()`), allowing tabs, split panes, and floating tool windows.
- **Theme**: Slate dark theme styled with rounded corners and high-contrast accents.
- **Main Menu Bar**:
  - `File`: New Scene, Open, Save, Save As, Exit.
  - `Edit`: Undo, Redo, Delete Selected Entity, Deselect.
  - `Scene`: Play, Pause, Step, Reset Camera View, Clear All.
  - `Entities`: Quick-create Empty Entity, Ball, Box, or Platform.
  - `View`: Toggle visibility for any active panel.
  - `Options`: Switch themes (Dark, Classic, Light), toggle Debug Grid, Colliders, or AABBs.
- **Shortcuts**:
  - `Space`: Toggle Play / Pause simulation.
  - `F`: Focus viewport camera on selected entity.
  - `Delete`: Delete currently selected entity.

---

### 4. Viewport & Rendering Pipeline

The **Scene Viewport Panel** (`SceneViewportPanel`) achieves full decoupling between the Editor UI and game graphics:
1. Dynamically resizes an off-screen `sf::RenderTexture` to match the ImGui window's available region.
2. Applies the [`EditorCamera`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/editor/include/EditorContext.hpp) view matrix (position and zoom level).
3. Renders the active scene via [`SimpleRenderer`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/SimpleRenderer.hpp).
4. Displays the resulting texture within ImGui using `ImGui::Image()`.
5. Viewport camera controls:
   - **Pan**: Hold **Right Mouse Button** or **Middle Mouse Button** and drag inside the viewport.
   - **Zoom**: Scroll the **Mouse Wheel** to zoom between 5% and 2000%.
   - **HUD Overlay**: On-screen coordinates and quick "Reset View" button.

---

### 5. Scene, Entities & Components

Game objects are represented by [`GameObject`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/GameObject.hpp):
- **Transform**: `position` (`Vec3`), `rotation` (`Quat`), `scale` (`Vec3`).
- **Physics**: Optional attached `std::unique_ptr<RigidBody>`.
- **Components**: Extensible component array (`std::vector<std::unique_ptr<Component>>`).
- **Lookup**: Query components via `entity->GetComponent<T>()` or check existence with `entity->HasComponent<T>()`.
- **Visuals**: Attach [`SpriteRenderer`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/SpriteRenderer.hpp) for color, rectangle sizes, or circle shapes.

In the **Entity Inspector**, modifying any transform parameter automatically invokes `entity->SyncTransform()`, instantly synchronizing the physics body's position, centroid, and orientation.

---

### 6. 2D Physics Simulation

The physics engine (`engine/physics/`) provides robust 2D dynamics:
- **RigidBody**: Mass, inverse mass (0 for static objects), linear and angular velocities, friction, restitution, and inertia tensors.
- **Colliders**: `Collider::CreateSphere(radius, density)` and `Collider::CreateBox(halfExtents, density)`.
- **BroadPhase**: Uniform spatial grid for rapid collision pair candidate pruning.
- **NarrowPhase**: Separating Axis Theorem (SAT) for box-box, sphere-sphere, and box-sphere contact manifold generation.
- **Simulation Control**: Toggle between Edit Mode (physics paused, live manual transform positioning) and Play Mode (active physics stepping).

---

## Prerequisites & Dependencies

### System Requirements
- **OS**: Linux, Windows, or macOS
- **Compiler**: C++20 compliant compiler (GCC 11+, Clang 13+, MSVC 2022+)
- **Build System**: CMake 3.20+
- **Linux Packages**: `libx11-dev`, `libxrandr-dev`, `libxcursor-dev`, `libxi-dev`, `libudev-dev`, `libgl1-mesa-dev`

### Submodules & Fetched Dependencies
- **SFML 3.2.0**: Included as submodule under `external/SFML`
- **spdlog**: Included as submodule under `external/spdlog` (header-only mode)
- **Dear ImGui**: Automatically fetched via CMake (`v1.91.5-docking` branch)
- **ImGui-SFML**: Automatically fetched via CMake (`v3.0` tag)

---

## Building & Running

### 1. Clone with Submodules
```bash
git clone --recursive https://github.com/Devsoc-BPGC/ElysiumEngine.git
cd ElysiumEngine
```
*(If cloned without submodules, run `git submodule update --init --recursive`)*

### 2. Configure & Build
```bash
# Configure Release build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build engine, editor app, and dependencies
cmake --build build -j$(nproc)
```

### 3. Run the Application
```bash
./build/bin/ElysiumApp
```

### 4. Run Unit Tests
```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

---

## Developer & Contributor Guide

### Creating a New Layer

To create a gameplay layer, inherit from `Elysium::Layer`:

```cpp
#include <Elysium.hpp>

class GameLayer : public Elysium::Layer {
public:
  GameLayer() : Layer("GameLayer") {}

  void OnAttach() override {
    ELYSIUM_INFO("GameLayer attached!");
  }

  void OnUpdate(Elysium::Timestep ts) override {
    // Run gameplay logic, update timers, move entities
  }

  void OnImGuiRender() override {
    // Optional in-game debug HUD
  }

  void OnEvent(Elysium::Event& event) override {
    // Handle gameplay events
  }
};
```
Push the layer in your application: `PushLayer(new GameLayer());`.

---

### Creating a Custom Editor Panel

All editor panels inherit from [`Elysium::EditorPanel`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/editor/include/EditorPanel.hpp):

```cpp
#include "EditorPanel.hpp"
#include <imgui.h>

namespace Elysium {

class MyCustomPanel : public EditorPanel {
public:
  MyCustomPanel() : EditorPanel("Custom Panel") {}

  void OnImGuiRender(EditorContext& context, Scene& scene, SimpleRenderer& renderer) override {
    if (ImGui::Begin(name.c_str(), &isOpen)) {
      ImGui::Text("Hello from custom panel!");
      ImGui::Text("Active entities: %zu", scene.objects.size());
    }
    ImGui::End();
  }
};

} // namespace Elysium
```
Register it in `EditorLayer`: `AddPanel<MyCustomPanel>();`.

---

### Creating a Custom Component

Inherit from [`Component`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/Component.hpp):

```cpp
#include "Component.hpp"
#include "GameObject.hpp"

class HealthComponent : public Component {
public:
  float currentHealth = 100.0f;
  float maxHealth = 100.0f;

  void Update(float dt) override {
    // Regenerate or check health
  }
};

// Attach to an entity:
auto entity = std::make_shared<GameObject>("Hero");
entity->AddComponent<HealthComponent>();
```

---

## Roadmap

Upcoming milestones as detailed in [`PLAN.md`](file:///home/mrgreenapple24/projects/ElysiumEngine/PLAN.md):
- [ ] **Scene Serialization**: YAML-based `.elyscene` scene saving and loading via `yaml-cpp`.
- [ ] **Renderer2D Batching**: Batched quad and circle drawing pipeline utilizing vertex buffers.
- [ ] **Entity Component System (ECS)**: Transitioning internal storage to `EnTT`.
- [ ] **Scripting Engine**: Embedding Lua via `sol2` for dynamic runtime gameplay scripting.
- [ ] **Audio Subsystem**: Integrating SFML's spatial audio module.

---

## License

Elysium Engine is open-source software licensed under the [MIT License](LICENSE).
