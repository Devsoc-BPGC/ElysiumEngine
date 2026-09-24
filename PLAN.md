# Elysium Engine — Development Plan (SFML Edition)

> A modular, data-driven 2D game engine built on **SFML** and **Dear ImGui** (via the `ImGui-SFML` binding).
> Revised from the earlier OpenGL/GLFW architecture — since the engine targets 2D only, SFML's own
> windowing, rendering, input, and audio modules replace the custom render-API abstraction, GLFW window
> layer, and OpenGL renderer entirely. This removes an entire layer of the original plan (Render API
> Abstraction) and simplifies several others.

---

## Migration Notes — What Changed From the OpenGL Plan

| Area | Old plan (OpenGL/GLFW) | Revised plan (SFML) |
|---|---|---|
| Windowing | Custom `IWindow` + GLFW, per-platform subclasses | `sf::RenderWindow`, one class, SFML is already cross-platform |
| Input | `glfwGetKey` polling + GLFW callbacks | `sf::Keyboard` / `sf::Mouse` polling + `sf::Event` |
| Rendering | Custom `RendererAPI` abstraction (OpenGL now, Vulkan/DX later) | Direct on top of `sf::RenderTarget`, `sf::VertexArray`, `sf::Shader` — no swappable backend, SFML *is* the backend |
| Framebuffers | Custom `Framebuffer` wrapping FBOs, second color attachment for ID-picking | `sf::RenderTexture` (built-in offscreen target); picking done via CPU hit-testing, not a GPU ID buffer |
| Shaders | Raw GLSL compiled/linked by hand | `sf::Shader` (still GLSL under the hood, but compiled/bound by SFML) |
| 3D rendering | Forward renderer, PBR materials, skybox, `EditorCamera` | **Removed.** SFML has no 3D pipeline; out of scope until/unless a separate module is added later |
| Audio | Not yet designed | `sf::Audio` module (`sf::Sound`, `sf::Music`) — no separate audio backend needed |
| ImGui backend | `imgui_impl_glfw` + `imgui_impl_opengl3` | `ImGui-SFML` (single binding, handles input forwarding + rendering) |
| ECS, Physics, Scripting, Memory, VFS | — | **Unchanged** — none of these depended on the graphics backend |

Core dependencies for this revision:
- **SFML 2.6+ / 3.x** (`graphics`, `window`, `system`, `audio` modules)
- **Dear ImGui** + **ImGui-SFML** (bridging layer)
- **EnTT** (ECS)
- **Sol2 + Lua 5.4** (scripting)
- **yaml-cpp** (scene/prefab serialization)
- **spdlog** (logging)

---

## Table of Contents

1. [Entry Point](#1-entry-point)
2. [Application Layer](#2-application-layer)
3. [Window Layer — Input & Events](#3-window-layer--input--events)
4. [Renderer](#4-renderer)
5. [Debugging Support](#5-debugging-support)
6. [Scripting Language](#6-scripting-language)
7. [Memory Systems](#7-memory-systems)
8. [Entity Component System](#8-entity-component-system)
9. [Physics Engine](#9-physics-engine)
10. [File I/O & Virtual File System](#10-file-io--virtual-file-system)

---

## 1. Entry Point

### Goal
Define a clean, platform-agnostic entry point that bootstraps the engine without leaking platform
details into application code. Largely unchanged from the original plan — SFML doesn't remove the
need for a defined entry point, it just removes the GL-context bootstrapping that used to happen here.

### Design
- The engine owns `main()`. The client application defines a `CreateApplication()` factory function.
- The engine calls `CreateApplication()`, runs the app loop, and cleans up.

### Implementation Steps
1. Create `Elysium/engine/core/src/EntryPoint.h`
   - Define `main()` here, guarded by `#ifdef ELYSIUM_PLATFORM_WINDOWS` etc. (kept only for things like
     `WinMain` vs `main`, DLL export macros, and file-path conventions — not for windowing, since SFML
     already abstracts that).
   - Call `Elysium::CreateApplication()` and invoke `app->Run()`.
2. Client-side application (e.g. `Sandbox`) only needs to implement:
```cpp
   #include <Elysium.h>
   Elysium::Application* Elysium::CreateApplication() {
       return new SandboxApp();
   }
```
3. Export the engine as a shared library (`ELYSIUM_API` macro using `__declspec(dllexport)` on
   Windows, `__attribute__((visibility("default")))` on GCC/Clang).
4. Link SFML statically or dynamically per-platform in the build system (CMake `find_package(SFML)`),
   no separate GLFW/GLAD dependency to manage anymore.

### Milestone
- A `Sandbox` project compiles and links against the engine DLL/SO with only an `#include <Elysium.h>`
  umbrella header and a `CreateApplication()` implementation.

---

## 2. Application Layer

### Goal
Provide a stable base class that manages the main loop, layer stack, ImGui overlay, and engine
subsystem lifetime.

### Design
- `Application` is a singleton-like base class.
- It owns the `Window` (wrapping `sf::RenderWindow`), the layer stack, and the ImGui layer.
- It drives the main loop with a fixed/variable timestep using `sf::Clock`.

### Key Classes
```
Application
├── LayerStack
│   ├── Layer (base)
│   └── Overlay (top of stack, always last)
└── ImGuiLayer (an Overlay)
```

### Implementation Steps
1. **Application class**
   - `Init()` — initialise subsystems in order: logging → memory → window → renderer → physics → ECS
   - `Run()` — main loop; `sf::Clock deltaClock` measures `DeltaTime` each frame
   - `Shutdown()` — reverse-order teardown
   - `OnEvent(Event&)` — dispatch events down the layer stack
2. **LayerStack**
   - `PushLayer(Layer*)` — adds below overlays
   - `PushOverlay(Layer*)` — adds above all layers
   - `PopLayer` / `PopOverlay`
   - Iterates front-to-back for updates, back-to-front for rendering
3. **Layer base class**
   - `OnAttach()`, `OnDetach()`, `OnUpdate(Timestep)`, `OnEvent(Event&)`, `OnImGuiRender()`
4. **Timestep**
   - Wrap `sf::Time` (from `deltaClock.restart()`) in a `Timestep` class with implicit `float`
     conversion and `GetMilliseconds()` / `GetSeconds()` helpers.
5. **ImGuiLayer**
   - `ImGui::SFML::Init(window)` on attach
   - Each frame: `ImGui::SFML::Update(window, deltaTime)` → layer `OnImGuiRender()` calls →
     `ImGui::SFML::Render(window)`
   - `ImGui::SFML::ProcessEvent(window, event)` must be fed every polled `sf::Event` — do this in the
     `Window`'s event pump before translating to internal engine events
   - `ImGui::SFML::Shutdown()` on detach
   - This single binding replaces the old `imgui_impl_glfw` + `imgui_impl_opengl3` pair.

### Milestone
- A layer can be pushed, updated each frame, and rendered with ImGui widgets drawn over an SFML scene.

---

## 3. Window Layer — Input & Events

### Goal
Wrap `sf::RenderWindow` in a thin, engine-facing interface, and translate `sf::Event` into the
engine's own event types so higher layers (and Lua scripts) never touch SFML directly.

### Design
- SFML is already cross-platform, so there is **no need for `WindowsWindow` / `LinuxWindow`
  subclasses** — a single `Window` class wrapping `sf::RenderWindow` suffices.
- Events are value types dispatched through a callback, translated from `sf::Event` at the point of
  polling. No dynamic allocation per event.
- Input polling (for held keys/buttons) is separate from event dispatch, same as before.

### Event System Architecture
```
Event (base, non-copyable)
├── WindowResizeEvent, WindowCloseEvent
├── KeyPressedEvent, KeyReleasedEvent, KeyTypedEvent (from sf::Event::TextEntered)
├── MouseMovedEvent, MouseScrolledEvent
└── MouseButtonPressedEvent, MouseButtonReleasedEvent
EventDispatcher  ← Matches event type and calls typed handler
```

**Dispatch pattern (unchanged):**
```cpp
EventDispatcher dispatcher(event);
dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
```

### Implementation Steps
1. **Event base class** with `EventType`, `EventCategory` bitmask (Input, Keyboard, Mouse,
   Application), `Handled` flag — unchanged from the original design.
2. **Concrete event classes** — each stores relevant data (key code, mouse position, etc.), sourced
   from the matching `sf::Event` fields.
3. **EventDispatcher** — templated `Dispatch<T>()` that checks type and invokes callback.
4. **Window class** (wraps `sf::RenderWindow` directly, no interface/factory split needed)
   - `Window(WindowProps)` — constructs `sf::RenderWindow` with a `sf::VideoMode` and `sf::ContextSettings`
   - `OnUpdate()` — polls all pending events via `window.pollEvent(sfEvent)`:
     1. Forward the raw `sf::Event` to `ImGui::SFML::ProcessEvent(window, sfEvent)`
     2. Translate it into the matching internal `Event` subclass
     3. Invoke the stored `EventCallbackFn`
     4. `window.display()` at the end of the frame
   - `SetEventCallback(fn)`, `SetVSync(bool)` (`window.setVerticalSyncEnabled`), `GetWidth/Height()`,
     `GetNativeHandle()` (`window.getSystemHandle()` when truly needed)
5. **Input class (polling)**
   - Static `IsKeyPressed(KeyCode)` → `sf::Keyboard::isKeyPressed(...)`
   - Static `IsMouseButtonPressed(MouseCode)` → `sf::Mouse::isButtonPressed(...)`
   - Static `GetMousePosition()` → `sf::Mouse::getPosition(window)`, converted to window-local /
     world-space coordinates as needed
   - `KeyCode` / `MouseCode` enums map directly to `sf::Keyboard::Key` / `sf::Mouse::Button`, so this
     layer is mostly a thin rename for engine-API stability (so scripts and game code aren't coupled
     to SFML's enum names directly).

### Milestone
- Window opens, `sf::Event`s dispatch correctly to layers (and to ImGui), keyboard/mouse polling
  works, ImGui widgets receive input correctly alongside gameplay input.

---

## 4. Renderer

### Goal
Build a batched, cache-friendly 2D renderer directly on top of SFML's graphics primitives —
`sf::RenderTarget`, `sf::VertexArray`, `sf::Texture`, `sf::Shader`, `sf::RenderTexture` — with no
separate render-API abstraction layer, since SFML is the only backend this engine targets.

> **Note on 3D:** SFML has no 3D rendering pipeline. The original plan's `Renderer3D` (forward
> rendering, PBR materials, lighting UBOs, skybox, `EditorCamera`) is dropped entirely. If 3D support
> is ever wanted later, it would live in a separate module rendering into its own OpenGL context —
> out of scope for this plan.

### Renderer Architecture
```
Renderer (high-level API, scene submission)
├── Renderer2D          ← Batched quads, circles, lines, text — built on sf::VertexArray
└── RenderCommand       ← Thin wrappers over sf::RenderTarget (Clear, SetClearColor, Draw)
```

### 4.1 Renderer2D — Batched Sprite/Shape Renderer

**Key constraint vs. the OpenGL plan:** SFML draw calls take exactly one bound texture and one
`sf::Shader` per `draw()` call — there's no multi-texture-slot trick like `GL_MAX_TEXTURE_IMAGE_UNITS`.
So batching keys on **(texture, shader, blend mode)** rather than filling N texture slots:

- Sprites sharing the same texture (ideally a **texture atlas** built at asset-import time) batch
  together into one `sf::VertexArray` and flush in a single `draw()` call.
- Changing texture, shader, or blend mode forces a flush of the current batch before starting a new one.
- Recommending **texture atlasing** up front (pack related sprites into one `sf::Texture`) is the
  single biggest lever for keeping batch counts low, since it directly replaces what texture-array
  slotting did in the OpenGL plan.

**Data flow:**
1. `Renderer2D::BeginScene(camera)` — computes a view-projection transform from the camera and applies
   it as the target's `sf::View`, or bakes it into per-vertex transforms if a shader-driven camera is
   preferred (see 4.3).
2. User calls `DrawQuad(transform, texture, tint)`, `DrawCircle(...)`, `DrawLine(...)`, `DrawText(...)`.
3. Internally appends to a CPU-side `sf::VertexArray` (`sf::PrimitiveType::Triangles`) keyed by the
   current (texture, shader, blend mode).
4. When the batch key changes, or `EndScene()` is called — flush: `target.draw(vertexArray, states)`.
5. `Renderer2D::EndScene()` — flushes all remaining batches, resets state.

**Implementation Steps:**
1. Quad batch: pre-allocated `QuadVertex[]` array (position, color, UV) grouped by texture key.
2. `BatchKey` struct: `{ const sf::Texture*, const sf::Shader*, sf::BlendMode }`, used as a map key to
   group draw calls before flush.
3. Circle and line batches (own vertex layout / primitive type per shape kind).
4. Renderer2D statistics: draw calls, quad count, batch-break count per frame (shown in an ImGui
   debug panel — batch-break count is the SFML-specific metric worth watching closely).
5. Camera abstraction: `OrthographicCamera` (wraps an `sf::View`), `OrthographicCameraController`
   (pan/zoom, drives the `sf::View` each frame).

### 4.2 Framebuffer / Offscreen Rendering

- Use `sf::RenderTexture` directly as the offscreen target — no custom FBO wrapper needed.
- Resizable: recreate the `sf::RenderTexture` on viewport resize (SFML render textures aren't
  resizable in place).
- Used by the editor viewport: render the scene into the `sf::RenderTexture`, then display its
  `getTexture()` via `ImGui::Image()`.
- **Entity picking is reworked** since SFML render textures don't support multiple color attachments
  (no GPU integer ID buffer like the old plan's second attachment):
  - Convert the mouse's screen position to world space via the camera's inverse view transform
    (`window.mapPixelToCoords()` is the SFML-native helper for this).
  - Hit-test against entities' bounding boxes (from `SpriteRendererComponent` size or collider shape)
    in the ECS, picking the topmost hit by render order.
  - This is simpler than the GPU-picking approach and is plenty fast for 2D scenes with reasonable
    entity counts; revisit only if profiling shows it's a bottleneck at very high entity counts.

### 4.3 Shader System

1. `Shader` class wraps `sf::Shader`: `loadFromFile(vertPath, fragPath)`, `setUniform(name, value)`
   (SFML's own uniform API — no manual `glGetUniformLocation` caching needed, SFML handles this).
2. `ShaderLibrary`: load from file, cache by name, hot-reload on file change (see §5).
3. Since SFML doesn't provide UBOs, camera and per-frame data are pushed as regular uniforms
   (`u_ViewProjection` as a `sf::Glsl::Mat4`) once per batch flush rather than once per frame via a
   bound buffer — an acceptable trade-off at 2D-appropriate draw-call counts.
4. Default shaders ship for: textured quad, flat-color quad, circle (SDF-based edge softening), line.

### Milestone
- Editor viewport renders a 2D scene (sprites, tilemaps, shapes) into an `sf::RenderTexture` displayed
  in an ImGui panel, with click-to-select entity picking working via CPU hit-testing.

---

## 5. Debugging Support

### Goal
Provide first-class logging, runtime assertions, profiling, and an in-engine debug overlay so
problems surface immediately during development. Unchanged from the original plan except for the
removal of the OpenGL-specific debug callback, since SFML doesn't expose a raw GL debug context by
default.

### 5.1 Logging — spdlog
1. Wrap spdlog in a `Log` class with two loggers: **Core** (engine-internal) and **Client** (game code).
2. Macros that compile to no-ops in distribution builds:
```cpp
   ELYSIUM_CORE_TRACE(...)
   ELYSIUM_CORE_INFO(...)
   ELYSIUM_CORE_WARN(...)
   ELYSIUM_CORE_ERROR(...)
   ELYSIUM_CORE_CRITICAL(...)
   ELYSIUM_TRACE(...)   // client
```
3. Log to console (colored) and to a rotating file sink.

### 5.2 Assertions
```cpp
#define ELYSIUM_ASSERT(x, ...) if (!(x)) { ELYSIUM_ERROR(__VA_ARGS__); __debugbreak(); }
#define ELYSIUM_CORE_ASSERT(x, ...) ...
```
- Disabled in `ELYSIUM_DIST` (distribution) builds.

### 5.3 Instrumentation Profiler
1. `Timer` class using `std::chrono::high_resolution_clock`.
2. `Instrumentor` — singleton, writes Chrome Tracing JSON (`chrome://tracing`) on session end.
3. `InstrumentationTimer` — RAII scope timer, reports to `Instrumentor`.
4. Macros:
```cpp
   ELYSIUM_PROFILE_BEGIN("Session", "profile.json")
   ELYSIUM_PROFILE_END()
   ELYSIUM_PROFILE_SCOPE("name")
   ELYSIUM_PROFILE_FUNCTION()   // uses __FUNCSIG__ / __PRETTY_FUNCTION__
```
5. Strip to no-ops outside of `ELYSIUM_PROFILE` build config.

### 5.4 ImGui Debug Panels
- **Renderer Stats panel**: draw calls, batch-break count, vertex count, frame time
- **Entity inspector panel**: selected entity's components, editable fields
- **Memory panel**: allocator stats (§7)
- **Console panel**: display log output inside ImGui
- **Physics debugger**: visualize colliders, broadphase AABBs, contact points, drawn as extra
  `Renderer2D` line batches (togglable)

### 5.5 Rendering Error Surfacing
- SFML doesn't expose `glDebugMessageCallback` directly; instead, wrap risky calls (shader
  compilation, texture loads, render-texture creation) and check their SFML-reported success
  booleans (`sf::Shader::loadFromFile` returns `bool`), routing failures through
  `ELYSIUM_CORE_ERROR`.
- If deeper GPU-level debugging is ever needed, an explicit `sf::ContextSettings` with
  `attributeFlags = sf::ContextSettings::Debug` can request a debug GL context, but this is not
  needed for the initial milestones.

### Milestone
- All engine paths log appropriately; profiling sessions produce loadable JSON; shader/texture load
  failures are caught and logged immediately instead of failing silently.

---

## 6. Scripting Language

### Goal
Embed Lua (via Sol2) as the primary scripting language, allowing gameplay logic to be written without
recompiling the engine. **Unchanged from the original plan** — scripting doesn't depend on the
rendering backend.

### Design
- Lua scripts are attached to entities as a `ScriptComponent`.
- The engine binds engine types (`Entity`, `Input`, `Vec2`, `Transform`, etc.) to Lua — `Vec3` is
  dropped from the default bindings since this is a 2D-only engine; add it back only if a use case
  requires z-ordering math beyond a simple float layer index.
- Scripts implement lifecycle hooks: `OnCreate`, `OnUpdate(dt)`, `OnDestroy`, `OnCollision`.

### Implementation Steps
1. **Sol2 integration**
   - Embed Lua 5.4 (or LuaJIT for performance).
   - Create `sol::state` as part of the `ScriptingEngine` singleton.
   - Open standard safe libs: `base`, `math`, `string`, `table`, `io` (sandboxed).
2. **Engine API bindings**
```lua
   -- Example script
   function OnCreate()
       self:GetComponent("RigidBody").velocity = Vec2(0, 5)
   end
   function OnUpdate(dt)
       if Input.IsKeyPressed(Key.Space) then ... end
   end
```
   - Bind `Entity`, `TransformComponent`, `RigidBodyComponent`, `Input`, `Log`, `Vec2`, `Mat3` (2D
     affine transform, in place of `Mat4`)
   - Bind `Scene` — `CreateEntity()`, `DestroyEntity()`, `FindEntityByName()`
3. **ScriptComponent**
   - Stores script file path and a `sol::table` (the script's `self` instance).
   - `ScriptingEngine::OnCreateEntity(entity)` — loads the Lua file, instantiates a table, calls
     `OnCreate`.
   - `ScriptingEngine::OnUpdateEntity(entity, dt)` — calls `OnUpdate(dt)` per frame.
4. **Hot reloading**
   - Watch script files for modifications (see §10 file watcher).
   - On change: call `OnDestroy`, re-execute file, call `OnCreate`.
5. **Error handling**
   - Wrap all Lua calls in `sol::protected_function` and route errors to the engine logger.
6. **Script editor integration (ImGui)**
   - File browser panel to assign scripts to entities.
   - Basic in-editor Lua syntax highlighting (via `ImGui::InputTextMultiline` + a simple lexer, or
     embed a text editor library like ImGuiColorTextEdit).

### Future Extension
- C# scripting via Mono (more complex, deferred to a later milestone).

### Milestone
- An entity with a `ScriptComponent` runs Lua that reads input and modifies a `RigidBodyComponent`
  each frame.

---

## 7. Memory Systems

### Goal
Replace uncontrolled heap allocation with purpose-built allocators that improve cache efficiency,
reduce fragmentation, enable leak detection, and give insight into memory usage. **Unchanged from the
original plan.**

### 7.1 Allocator Types

| Allocator | Use Case |
|---|---|
| Linear / Arena | Per-frame scratch space, temporary parsing buffers |
| Stack | Nested scope allocations, undo/redo state |
| Pool | Fixed-size objects: entities, components, events, particles |
| Free-List (General) | Variable-size long-lived allocations, fallback |
| TLSF (optional) | Real-time safe general allocator with bounded fragmentation |

### Implementation Steps
1. **Base `IAllocator` interface**: `Allocate(size, alignment)`, `Deallocate(ptr, size)`, `Reset()`
   (where applicable).
2. **LinearAllocator**: offset pointer into a pre-allocated block; `Reset()` returns to zero; no
   per-item free.
3. **StackAllocator**: like linear but with a marker stack to pop frames.
4. **PoolAllocator**: slab of N fixed-size blocks; free list of available slots; O(1) alloc/free;
   template-typed variant `PoolAllocator<T, N>`.
5. **FreeListAllocator**: doubly-linked free block list with header metadata; first-fit or best-fit
   policy; coalescing on free.
6. **MemoryManager singleton**
   - Owns a large backing block (e.g. 256 MB virtual reservation — smaller than the 3D plan's 512 MB
     since there's no mesh/texture-heavy 3D asset pipeline to budget for).
   - Hands out sub-arenas to each subsystem.
   - Tracks per-subsystem usage, peak usage, allocation count.
7. **Overriding `new`/`delete`** (optional but powerful):
   - Provide a global `operator new` that routes through the engine allocator.
   - Tag allocations with source file/line in debug builds.
8. **Leak detection**
   - In debug builds maintain an `std::unordered_map<void*, AllocationInfo>` tracking every live
     allocation.
   - On shutdown, log all live allocations as leaks with file/line/size.
9. **ImGui memory panel**: bar graph of allocator usage vs capacity per subsystem.

### Milestone
- All engine subsystems allocate through named allocators; per-frame scratch allocations leave zero
  fragmentation; leaks are reported on shutdown.

---

## 8. Entity Component System

### Goal
Use EnTT as the ECS backbone, wrapped in an ergonomic engine-level API, with components trimmed down
to what a 2D engine actually needs.

### Design
- `Scene` owns an `entt::registry`.
- `Entity` is a thin wrapper around `entt::entity` + a pointer to its owning `Scene`.
- Components are plain data structs (no virtual functions).
- Systems are free functions or classes that query the registry.

### Built-in Components

| Component | Data |
|---|---|
| `TagComponent` | `std::string Name` |
| `TransformComponent` | `Vec2 Position`, `float Rotation`, `Vec2 Scale`; `GetTransform() -> Mat3` |
| `SpriteRendererComponent` | `Texture2DRef`, `Vec4 Color`, `float TilingFactor`, `int SortingLayer` |
| `CircleRendererComponent` | `Vec4 Color`, `float Thickness`, `float Fade` |
| `CameraComponent` | `OrthographicCamera`, `bool Primary`, `bool FixedAspect` — `PerspectiveCamera` dropped, this engine is 2D-only |
| `ScriptComponent` | `std::string ScriptPath`, `sol::table Instance` |
| `RigidBodyComponent` | `BodyType (Static/Kinematic/Dynamic)`, `Vec2 Velocity`, `float Mass` |
| `BoxCollider2DComponent` | `Vec2 Offset, Size`, `float Density, Friction, Restitution` |
| `CircleCollider2DComponent` | `Vec2 Offset`, `float Radius`, physics material params |
| `AudioSourceComponent` | `sf::SoundBuffer` ref (or `sf::Music` for streamed audio), volume, pitch, loop — SFML's audio module is used directly rather than a separate third-party audio backend |

> Dropped from the original plan: `MeshComponent`, `PointLightComponent`, `DirectionalLightComponent`
> — all 3D-only concepts with no SFML equivalent. If simple 2D lighting is wanted later, it's better
> modeled as a shader effect (a light-mask texture blended additively) than as its own component type,
> and can be added without disturbing this list.

### Implementation Steps
1. **Scene class**
   - `CreateEntity(name)` → wraps `registry.create()` + adds `TagComponent` + `TransformComponent`
   - `DestroyEntity(entity)` → `registry.destroy()`
   - `OnUpdate(Timestep)` — run all systems
   - `OnRender()` — query render components (ordered by `SortingLayer`) and submit to `Renderer2D`
   - `OnViewportResize(w, h)` — update camera aspect ratios / `sf::View` sizes
2. **Entity class**
   - Templated `AddComponent<T>(args...)`, `GetComponent<T>()`, `HasComponent<T>()`,
     `RemoveComponent<T>()`
3. **Systems** (standalone functions iterating registry views)
   - `ScriptSystem::OnUpdate(registry, dt)`
   - `PhysicsSystem::OnUpdate(registry, dt)` — feeds physics engine
   - `RenderSystem::OnRender(registry)` — submits draw calls to `Renderer2D`
   - `AudioSystem::OnUpdate(registry, dt)` — starts/stops `sf::Sound` instances per `AudioSourceComponent`
4. **Scene serialization / deserialization** (YAML via yaml-cpp)
   - `SceneSerializer::Serialize(scene, filepath)` — write all entities and components to YAML
   - `SceneSerializer::Deserialize(filepath)` — reconstruct scene from YAML
5. **Scene hierarchy** (parent/child relationships)
   - `RelationshipComponent`: parent entity handle, first child, next sibling
   - Transform system: compute world transform from local + parent world transform (2D affine, `Mat3`)
6. **Prefabs**
   - Serialize a single entity subtree to a YAML prefab file.
   - `Scene::InstantiatePrefab(filepath, parent)` — deserialize and attach.

### Editor Integration
- Hierarchy panel: tree view of all entities, drag to reparent
- Inspector panel: display and edit all components via ImGui; `AddComponent` dropdown
- Scene panel: select entities by clicking in the viewport, using the CPU hit-test picking from §4.2

### Milestone
- A scene with 10,000 entities with transform + sprite components renders and updates at 60 fps. Save
  and load round-trip produces identical scenes.

---

## 9. Physics Engine

### Goal
Build a robust, feature-complete 2D physics simulation. **Entirely unaffected by the SFML change** —
this section is carried over as-is, minus the "Optional 3D Physics" subsection, which no longer
applies to an engine that has no 3D renderer to visualize it with.

### 9.1 Architecture
```
PhysicsEngine2D (singleton or subsystem)
├── PhysicsWorld2D          ← owns all bodies, broadphase, solver
│   ├── BroadPhase          ← AABB tree / sweep-and-prune
│   ├── NarrowPhase         ← SAT / GJK collision detection
│   ├── ConstraintSolver    ← Sequential impulse (Erin Catto method)
│   └── RigidBody2D[]
└── PhysicsDebugDraw        ← renders colliders via Renderer2D lines
```

### 9.2 Core Features to Build

**Rigid Body Dynamics**
- Body types: Static, Kinematic, Dynamic
- Linear & angular velocity, damping
- Force, impulse, torque application
- Mass & inertia tensor computation from shape

**Collider Shapes**
- Circle, AABB (box), OBB (oriented box), Polygon (convex hull), Capsule
- Compound colliders (multiple shapes per body)
- Sensor / trigger zones (detect overlap, no impulse response)

**Broadphase**
- Dynamic AABB Tree (BVH) — O(n log n) pair generation
- Persistent contact manifold caching to avoid redundant narrowphase tests

**Narrowphase**
- SAT (Separating Axis Theorem) for polygon-polygon and polygon-circle
- GJK + EPA for general convex shapes
- Contact point generation, penetration depth, collision normal

**Constraint Solver (Sequential Impulse)**
- Velocity-level constraint formulation
- Baumgarte position stabilisation
- Configurable iteration count (default 10)
- Joints: distance, revolute (hinge), prismatic, spring

**Material System**
- Per-collider: density, friction (static + dynamic), restitution (bounciness)
- Material pair lookup table for mixed-material contacts

**Callbacks & Events**
- `OnCollisionBegin(EntityA, EntityB, ContactData)`
- `OnCollisionEnd(EntityA, EntityB)`
- `OnTriggerEnter/Exit`
- Route to `ScriptComponent` Lua callbacks

**Integration & Timestep**
- Fixed timestep integration (default 1/60 s) with accumulator
- Semi-implicit Euler integration
- Substep support for tunnelling prevention (fast-moving objects)

### 9.3 Integration with ECS
- `PhysicsSystem` reads `RigidBodyComponent` + `BoxCollider2DComponent` / `CircleCollider2DComponent`
  from the registry.
- On scene start: create physics bodies and register in `PhysicsWorld2D`.
- On scene stop: destroy all physics state.
- Each frame: `PhysicsWorld2D::Step(fixedDt)` → write back positions/rotations to
  `TransformComponent`.

### 9.4 Physics Debugger
- Toggle-able via ImGui panel.
- Render all collider outlines, velocity vectors, contact normals using `Renderer2D` lines.
- Display body count, active island count, contact count.

### Milestone
- A scene with 500 dynamic boxes stacked and falling simulates at 60 fps with stable stacking,
  correct restitution, and triggers firing Lua callbacks.

---

## 10. File I/O & Virtual File System

### Goal
Provide a unified, platform-agnostic file access layer that abstracts over real disk paths, supports
asset packaging (PAK files), and enables hot-reloading of assets. **Unchanged from the original
plan** — this layer has no dependency on the rendering backend.

### 10.1 Virtual File System (VFS) Design
```
VFS
├── MountPoint map: { "assets://", "shaders://", "scripts://" }
│   Each maps to one or more IFileProvider:
│   ├── DiskFileProvider     ← reads from a real directory
│   └── PackFileProvider     ← reads from a PAK archive
└── FileHandle
```
- Paths in engine code always use virtual paths: `"assets://textures/player.png"`.
- VFS resolves them through the mount table at runtime.
- Multiple providers can be stacked on one mount point (archive overlays real dir, or vice versa).

### 10.2 Implementation Steps
1. **Path utilities**
   - `Path` type (thin `std::string` wrapper or `std::filesystem::path`)
   - Normalize separators, resolve `.` and `..`, split extension, split stem
2. **IFileProvider interface**
   - `Open(virtualPath) -> FileHandle`
   - `Exists(virtualPath) -> bool`
   - `List(virtualPath) -> std::vector<Path>`
3. **DiskFileProvider**
   - Maps a virtual root to a filesystem directory.
   - Uses `std::fstream` or OS APIs for reads/writes.
   - `ReadAll(path) -> std::vector<uint8_t>`
   - `WriteAll(path, data)`
4. **VFS class**
   - `Mount(virtualRoot, IFileProvider*)` — register provider
   - `Unmount(virtualRoot)`
   - `Open(virtualPath, mode) -> FileHandle`
   - `ReadTextFile(virtualPath) -> std::string`
   - `ReadBinaryFile(virtualPath) -> std::vector<uint8_t>`
5. **Async I/O**
   - `VFS::ReadAsync(path, callback)` — submits to a thread pool, calls callback on completion.
   - Used by the asset manager to load textures and audio clips in the background.
6. **PAK Archive format**
   - Simple custom format: header + file table (name hash → offset + size) + compressed data
     (zstd or zlib).
   - `PackBuilder` tool: pack a directory tree into a `.pak` file.
   - `PackFileProvider`: memory-maps the PAK, serves files from it.
7. **Asset Manager** (built on VFS)
   - `AssetManager::Load<Texture2D>("assets://textures/player.png")` → returns `Ref<Texture2D>`,
     internally an `sf::Texture` loaded via `loadFromMemory` on the VFS-read bytes (so PAK-packed
     textures load with no extra code path vs. loose files).
   - Caches by path; returns existing handle if already loaded.
   - Reference-counted: unloads when last `Ref` is dropped.
   - `AssetHandle` type (UUID) for serialisation in scene files (instead of storing raw paths).
8. **File Watcher (Hot Reload)**
   - Background thread polls file modification times (or uses OS `inotify` /
     `ReadDirectoryChangesW`).
   - On change: notify registered watchers.
   - Shader hot-reload, texture hot-reload, Lua script hot-reload hook in here.

### 10.3 Serialisation Formats

| Data | Format |
|---|---|
| Scenes, prefabs | YAML (`yaml-cpp`) |
| Engine config | INI or YAML |
| Textures | Standard `.png` / `.jpg`, loaded directly via `sf::Texture::loadFromMemory` (SFML's own image loader — no separate GPU-compressed texture format needed for a 2D sprite pipeline) |
| PAK archives | Custom `.elypak` |

### Milestone
- All engine assets are loaded through virtual paths; a PAK archive can replace the loose file
  directory transparently; shader and texture file changes hot-reload without restarting.

---

## Development Phases & Suggested Order

Dropping the render-API abstraction, GLFW window layer, and the entire 3D pipeline shortens the
foundation and renderer phases considerably compared to the original OpenGL plan.

### **Phase 1 — Foundation (Weeks 1–3)**
Entry Point → Application Layer → Window (SFML) & Events → Logging & Assertions → clear-screen +
first sprite drawn to the window.

### **Phase 2 — Core Renderer (Weeks 4–6)**
Renderer2D batching → Shader system (`sf::Shader`) → `sf::RenderTexture` offscreen target →
ImGui-SFML integration → Editor viewport panel.

### **Phase 3 — Editor Shell (Weeks 7–8)**
ImGui docking layout → Hierarchy panel → Inspector panel → Scene serialization (YAML) → Asset
drag-and-drop.

### **Phase 4 — ECS & Scene (Weeks 9–10)**
EnTT integration → Built-in components → Scene systems → Prefab system → CPU-based entity picking.

### **Phase 5 — Physics & Scripting (Weeks 11–13)**
Physics engine (broadphase, narrowphase, solver) → ECS physics integration → Lua + Sol2 embedding →
Script component → Physics debug draw.

### **Phase 6 — Memory & File Systems (Week 14)**
Allocator library → MemoryManager → VFS + DiskProvider → AssetManager + caching → File watcher +
hot-reload.

### **Phase 7 — Polish & Tooling (Week 15+)**
PAK packaging → Audio system (built on `sf::Audio`) → Profiler UI → Distribution build pipeline →
Documentation.

---

*Elysium Engine — Built from scratch on SFML, built to last.*