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
  - [7. Audio Subsystem & Spatial Sound](#7-audio-subsystem--spatial-sound)
  - [8. Asset Management & Registry](#8-asset-management--registry)
  - [9. Project Architecture & Native File Dialogs](#9-project-architecture--native-file-dialogs)
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
- **64-Bit UUID & Safe Entity Handles**:
  - High-performance `UUID` class backed by hardware entropy (`std::random_device`) and standard 64-bit Mersenne Twister (`std::mt19937_64`).
  - Persistent, unique 64-bit identifiers assigned to all `GameObject`s with full support for explicit IDs during scene deserialization.
  - O(1) hash map scene lookups and safe deletion (`Scene::GetEntityByUUID`, `Scene::DestroyEntityByUUID`).
  - Safe, lightweight templated `EntityHandle` reference wrapper preventing dangling pointer crashes.
- **Asset Manager & Registry (`AssetManager`)**:
  - Centralized static/singleton system mapping 64-bit UUID `AssetHandle`s to assets (`sf::Texture`, `sf::Font`, `sf::SoundBuffer`, `Scene`).
  - Thread-safe dual-mutex asset caching eliminating redundant disk reads or GPU/audio memory allocations.
  - Manifest serialization (`assets.meta`) persisting asset UUID-to-path relationships across engine sessions.
- **Interactive Content Browser Panel (`ContentBrowserPanel`)**:
  - ImGui panel inheriting from `EditorPanel` with folder tree navigation on the left and directory content grid on the right.
  - Navigation history stack (Back, Forward, Up), path breadcrumbs, search filtering, and thumbnail resizing.
  - Procedural visual icons tailored for folders, textures, audio files, fonts, and scene documents.
  - Automatic filesystem watcher and manual refresh button.
  - Drag-and-drop source with payload support (`CONTENT_BROWSER_ITEM_TEXTURE`, `CONTENT_BROWSER_ITEM_AUDIO`, etc.) received directly by component inspectors (e.g. dragging `.png` into `SpriteRenderer`).
- **Project Management System (`Project` & `ProjectSerializer`)**:
  - Dynamic project architecture driven by `.elyproj` project files tracking active project directories, asset roots, and start scenes.
  - Automatic project directory initialization creating `<Project>/Assets/`, `<Project>/Assets/Scenes/`, and `<Project>/Assets/assets.meta`.
  - Canonical absolute path resolution removing hardcoded workspace dependencies.
- **Cross-Platform Native File Dialogs (`FileDialog`)**:
  - Native OS folder selector and file picker dialogs on Linux (Zenity/KDialog), Windows (Win32 `IFileDialog` / `GetOpenFileName`), and macOS (`osascript`).
  - Seamlessly integrated with `File -> New Project...` and `File -> Open Project...` in the editor menu bar.
- **2D Physics Simulation**:
  - Continuous impulse-based rigid body dynamics, broadphase spatial hash grid, and narrowphase SAT collision response.
- **Audio Subsystem**:
  - Decoupled `AudioEngine` singleton/application subsystem managing streamed background music (`sf::Music`) and an asset-cached pool of sound effects (`sf::Sound` / `sf::SoundBuffer`).
  - `AudioSourceComponent` providing GameObject-attached 3D spatial audio synchronized with entity `Transform`s and SFML 3's `sf::Listener`.
  - Full editor reflection in the **Entity Inspector** with file path picker, volume/pitch sliders, loop/spatial toggles, and live playback preview controls.

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
│   │   │   ├── Application.hpp          # Application singleton and master loop
│   │   │   ├── AudioEngine.hpp          # High-level Audio Subsystem & sound pool
│   │   │   ├── AudioSourceComponent.hpp # GameObject spatial audio component
│   │   │   ├── Window.hpp               # sf::RenderWindow encapsulation and event pump
│   │   │   ├── Layer.hpp                # Base class for engine layers
│   │   │   ├── LayerStack.hpp           # Ordered layer and overlay stack
│   │   │   ├── Timestep.hpp             # Frame delta time wrapper (seconds/ms)
│   │   │   ├── KeyCodes.hpp             # Platform-independent key codes
│   │   │   ├── MouseCodes.hpp           # Platform-independent mouse codes
│   │   │   ├── GameObject.hpp           # Entity definition and component container
│   │   │   ├── EntityHandle.hpp         # Lightweight templated entity reference handle
│   │   │   ├── UUID.hpp                 # 64-bit UUID class and std::hash specialization
│   │   │   ├── Component.hpp            # Component base interface
│   │   │   ├── SpriteRenderer.hpp       # Visual shape/sprite renderer component
│   │   │   ├── Scene.hpp                # Scene container and physics world binding
│   │   │   ├── SimpleRenderer.hpp       # Off-screen & target-agnostic 2D renderer
│   │   │   ├── Input.hpp                # Static keyboard and mouse polling
│   │   │   ├── FileDialog.hpp           # Cross-platform native file and folder pickers
│   │   │   ├── Project.hpp              # Project configuration and active management
│   │   │   ├── ProjectSerializer.hpp    # YAML/JSON project serialization (.elyproj)
│   │   │   ├── Log.h                    # spdlog logging macros
│   │   │   ├── Core.hpp                 # Platform macros and DLL exports
│   │   │   ├── EntryPoint.hpp           # Platform main() bootstrap
│   │   │   └── Elysium.hpp              # Umbrella engine header for client apps
│   │   └── src/
│   │       ├── Application.cpp          # Engine game loop and event distribution
│   │       ├── AudioEngine.cpp          # Audio subsystem, sound pool & buffer cache
│   │       ├── AudioSourceComponent.cpp # AudioSourceComponent lifecycle & spatialization
│   │       ├── FileDialog.cpp           # Native OS dialog implementation (Linux/Win/Mac)
│   │       ├── GameObject.cpp           # GameObject lifecycle & logging
│   │       ├── Project.cpp              # Project initialization, loading, and directories
│   │       ├── ProjectSerializer.cpp    # .elyproj serialization and deserialization
│   │       ├── Scene.cpp                # Scene entity registry and physics integration
│   │       ├── UUID.cpp                 # 64-bit Mersenne Twister & entropy generator
│   │       ├── Window.cpp               # SFML 3 event pump and translation
│   │       ├── Layer.cpp                # Layer base implementation
│   │       ├── LayerStack.cpp           # LayerStack management
│   │       └── Log.cpp                  # spdlog initialization
│   ├── editor/                 # Dear ImGui Editor UI module
│   │   ├── include/
│   │   │   ├── AssetManager.hpp        # Centralized asset manager, registry, and caching
│   │   │   ├── EditorLayer.hpp         # Engine layer driving ImGui lifecycle & docking
│   │   │   ├── EditorContext.hpp       # Shared state (camera, selection, play state, stats)
│   │   │   ├── EditorPanel.hpp         # Abstract modular editor panel interface
│   │   │   ├── EditorLogSink.hpp       # spdlog memory sink for in-editor console
│   │   │   └── panels/                 # Concrete panel definitions
│   │   │       ├── ContentBrowserPanel.hpp   # Interactive directory tree & grid asset browser
│   │   │       ├── SceneViewportPanel.hpp    # Off-screen texture & pan/zoom camera
│   │   │       ├── SceneHierarchyPanel.hpp   # Entity list, search, add/delete
│   │   │       ├── EntityInspectorPanel.hpp  # Component reflection and mutation
│   │   │       ├── ToolbarPanel.hpp          # Play, Pause, Step controls
│   │   │       ├── ConsolePanel.hpp          # Filtered log viewer
│   │   │       └── StatsPanel.hpp            # Profiler, FPS plot, and timings
│   │   └── src/
│   │       ├── AssetManager.cpp        # Thread-safe caching and manifest serialization
│   │       ├── EditorLayer.cpp         # DockSpace, theme, menu bar, and shortcuts
│   │       ├── EditorLogSink.cpp       # Circular log buffer implementation
│   │       └── panels/                 # Panel implementations
│   │           └── ContentBrowserPanel.cpp   # Folder tree, grid, thumbnails, and drag-and-drop
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
    ├── AssetManagerTest.cpp
    ├── BoundaryTest.cpp
    ├── BroadPhasePerformanceTest.cpp
    ├── FrictionTest.cpp
    ├── PhysicsPerformanceTest.cpp
    ├── ProjectTest.cpp
    ├── RigidBodyDecouplingTest.cpp
    └── UUIDEntityTest.cpp
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
- **Identity & UUID**: Every `GameObject` is uniquely identified by a 64-bit [`UUID`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/UUID.hpp) (`entity->GetUUID()`). Supports explicit UUIDs upon creation for deserialization.
- **Transform**: `position` (`Vec3`), `rotation` (`Quat`), `scale` (`Vec3`).
- **Physics**: Optional attached `std::unique_ptr<RigidBody>`.
- **Components**: Extensible component array (`std::vector<std::unique_ptr<Component>>`).
- **Lookup**: Query components via `entity->GetComponent<T>()` or check existence with `entity->HasComponent<T>()`.
- **Visuals**: Attach [`SpriteRenderer`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/SpriteRenderer.hpp) for color, rectangle sizes, or circle shapes.

#### Scene Entity Registry
[`Scene`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/Scene.hpp) maintains an internal `std::unordered_map<UUID, std::shared_ptr<GameObject>>` for fast O(1) entity management:
```cpp
// Look up an entity by UUID
GameObject* entity = scene.GetEntityByUUID(uuid);

// Safely destroy an entity (unregisters physics, map entry, and list)
bool success = scene.DestroyEntityByUUID(uuid);
```

#### Safe Entity Handles (`EntityHandle`)
To prevent dangling pointers across frames when entities are destroyed, components can use the lightweight templated [`EntityHandle<T>`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/EntityHandle.hpp):
```cpp
// Store handle instead of raw pointer
Elysium::EntityHandle<GameObject> targetHandle(targetEntity->GetUUID());

// Safely resolve at runtime
if (GameObject* liveTarget = targetHandle.Resolve(&scene)) {
    // Entity is alive and safe to access
}

// Or resolve a specific component type
if (HealthComponent* health = targetHandle.ResolveComponent<HealthComponent>(&scene)) {
    health->TakeDamage(10.0f);
}
```

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

### 7. Audio Subsystem & Spatial Sound

Elysium features a decoupled, high-level audio system built with **SFML 3.2**:

- **AudioEngine Subsystem**:
  - Accessible via `AudioEngine::Get()` or `Application::Get().GetAudioEngine()`.
  - Background music streaming via `sf::Music` (`PlayMusic`, `PauseMusic`, `ResumeMusic`, `StopMusic`).
  - Cached sound buffers (`sf::SoundBuffer`) and dynamic sound pool recycling (`AcquireSound`, `PlaySound`).
  - Master volume control modulating SFML 3's `sf::Listener::setGlobalVolume`.

- **AudioSourceComponent**:
  - Attach audio playback directly to entities:
    ```cpp
    auto entity = std::make_shared<GameObject>("AudioEmitter");
    auto& audio = entity->AddComponent<Elysium::AudioSourceComponent>("assets/sounds/ambient.ogg");
    audio.isSpatial = true;
    audio.loop = true;
    audio.playOnAwake = true;
    audio.minDistance = 2.0f;
    audio.attenuation = 1.0f;
    ```
  - In spatial mode (`isSpatial = true`), the component tracks the entity's 3D transform (`Vec3 position`) and updates `sf::SoundSource::setPosition` in real-time relative to the global listener.

- **Editor Reflection**:
  - Exposes sound file selection, live play/pause/stop previews, volume/pitch sliders, and spatial attenuation parameters inside the **Entity Inspector**.

---

### 8. Asset Management & Registry

Elysium provides a centralized, thread-safe asset management pipeline centered on 64-bit [`UUID`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/UUID.hpp) handles:

- **Asset Handles & Metadata**:
  - Every asset is identified by an [`AssetHandle`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/editor/include/AssetManager.hpp) (`UUID`).
  - [`AssetMetadata`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/editor/include/AssetManager.hpp) stores the handle, `AssetType` (`Texture`, `Font`, `Audio`, `Scene`), normalized relative file path, and memory residency state.
  - Deduced automatically from file extensions (`.png`, `.jpg`, `.wav`, `.ogg`, `.ttf`, `.elyscene`).

- **Thread-Safe Caching (`Load<T>`)**:
  - Assets are cached in memory using dual-mutex synchronization, preventing redundant disk I/O and GPU/audio allocations:
    ```cpp
    // Fast O(1) cached lookup or thread-safe lazy load
    std::shared_ptr<sf::Texture> texture = AssetManager::Get().Load<sf::Texture>(handle);
    ```

- **Manifest Persistence (`assets.meta`)**:
  - Automatically reads and serializes the asset manifest across sessions, resolving relative project paths to persistent 64-bit UUIDs across engine and editor restarts.

- **Interactive Content Browser (`ContentBrowserPanel`)**:
  - Split-view editor panel featuring an interactive folder tree on the left and an adaptive thumbnail grid on the right.
  - Complete navigation history (Back, Forward, Up), path breadcrumbs, search filtering, and thumbnail scaling.
  - Drag-and-drop source using engine payloads (`CONTENT_BROWSER_ITEM_TEXTURE`, `CONTENT_BROWSER_ITEM_AUDIO`, etc.) received directly by component slots (e.g., dropping `.png` onto a `SpriteRenderer` slot).

---

### 9. Project Architecture & Native File Dialogs

Elysium organizes games into standalone, portable projects decoupled from the engine binary:

- **Project Configuration (`ProjectConfig`)**:
  - Encapsulates `Name`, `ProjectDirectory` (canonical absolute path), `AssetDirectory` (relative to project, defaults to `Assets`), and `StartScenePath`.
  - Serialized to/from `.elyproj` files using [`ProjectSerializer`](file:///home/mrgreenapple24/projects/ElysiumEngine/engine/core/include/ProjectSerializer.hpp).

- **Project Creation & Directory Initialization**:
  - Creating a new project automatically establishes the full workspace hierarchy:
    ```text
    /path/to/MyGame/
    ├── MyGame.elyproj        # Project configuration
    └── Assets/               # Project-isolated asset root
        ├── assets.meta       # Initialized registry manifest
        └── Scenes/           # Default scene storage
    ```

- **Cross-Platform Native File Dialogs (`FileDialog`)**:
  - Native OS directory selector and file picker dialogs without external dependencies:
    - **Linux**: Invokes native `zenity` (or `kdialog` fallback).
    - **Windows**: Win32 `IFileDialog` / `GetOpenFileNameA` and `SHBrowseForFolderA`.
    - **macOS**: Native `osascript` AppleScript dialogs.
  - Triggered directly from the Editor menu:
    - `File -> New Project...` (`Ctrl+Shift+N`): Select any folder on disk to initialize a project.
    - `File -> Open Project...` (`Ctrl+Shift+O`): Browse and load existing `.elyproj` files.
  - Dynamically updates the window title, dockspace header, and reloads `AssetManager` and `ContentBrowserPanel` to the selected project root.

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
- [x] **Audio Subsystem**: Integrating SFML 3.2's spatial audio module, buffer caching, and sound pooling.
- [x] **Asset Registry & Cache**: 64-bit UUID asset handles, thread-safe memory caching, and manifest serialization (`assets.meta`).
- [x] **Content Browser Panel**: Directory tree navigation, thumbnail grid, file watcher, and drag-and-drop integration.
- [x] **Project Management & Native Dialogs**: Standalone `.elyproj` project files, directory auto-initialization, and OS file/folder pickers.

---

## License

Elysium Engine is open-source software licensed under the [MIT License](LICENSE).
