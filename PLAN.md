# Elysium Engine — Development Plan

> A modular, data-driven 2D/3D game engine built on OpenGL and Dear ImGui.
> Migrating from an SFML-based 2D prototype to a fully-featured, production-grade engine architecture.

---

## Table of Contents

1. [Entry Point](#1-entry-point)
2. [Application Layer](#2-application-layer)
3. [Window Layer — Input & Events](#3-window-layer--input--events)
4. [Renderer](#4-renderer)
5. [Render API Abstraction](#5-render-api-abstraction)
6. [Debugging Support](#6-debugging-support)
7. [Scripting Language](#7-scripting-language)
8. [Memory Systems](#8-memory-systems)
9. [Entity Component System](#9-entity-component-system)
10. [Physics Engine](#10-physics-engine)
11. [File I/O & Virtual File System](#11-file-io--virtual-file-system)

---

## 1. Entry Point

### Goal
Define a clean, platform-agnostic entry point that bootstraps the engine without leaking platform details into application code.

### Design

- The engine owns `main()`. The client application defines a `CreateApplication()` factory function.
- The engine calls `CreateApplication()`, runs the app loop, and cleans up.

### Implementation Steps

1. Create `Elysium/engine/core/src/EntryPoint.h`
   - Define the `main()` function here using `#ifdef ELYSIUM_PLATFORM_WINDOWS` etc.
   - Call `Elysium::CreateApplication()` and invoke `app->Run()`
2. Client-side application (e.g. `Sandbox`) only needs to implement:
```cpp
   #include <Elysium.h>
   Elysium::Application* Elysium::CreateApplication() {
       return new SandboxApp();
   }
```
3. Define platform macros (`ELYSIUM_PLATFORM_WINDOWS`, `ELYSIUM_PLATFORM_LINUX`) in the build system.
4. Export the engine as a shared library (`ELYSIUM_API` macro using `__declspec(dllexport)` on Windows, `__attribute__((visibility("default")))` on GCC/Clang).

### Milestone
- A `Sandbox` project compiles and links against the engine DLL/SO with only an `#include <Elysium.h>` umbrella header and a `CreateApplication()` implementation.

---

## 2. Application Layer

### Goal
Provide a stable base class that manages the main loop, layer stack, ImGui overlay, and engine subsystem lifetime.

### Design

- `Application` is a singleton-like base class.
- It owns the window, the layer stack, and the ImGui layer.
- It drives the main loop with a fixed/variable timestep.

### Key Classes
Application
├── LayerStack
│   ├── Layer (base)
│   └── Overlay (top of stack, always last)
└── ImGuiLayer (an Overlay)

### Implementation Steps

1. **Application class**
   - `Init()` — initialise all subsystems in order: logging → memory → window → renderer → physics → ECS
   - `Run()` — main loop with `DeltaTime` calculation using `std::chrono`
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
   - Wrap delta time in a `Timestep` class with implicit `float` conversion and `GetMilliseconds()` / `GetSeconds()` helpers.
5. **ImGuiLayer**
   - Init ImGui context, set style, connect GLFW + OpenGL backends
   - `Begin()` / `End()` called around all `OnImGuiRender()` calls

### Milestone
- A layer can be pushed, updated each frame, and rendered with ImGui widgets.

---

## 3. Window Layer — Input & Events

### Goal
Abstract OS windows and input behind platform-agnostic interfaces. Replace the existing SFML input manager with an event-driven system.

### Design

- `Window` is an interface; `WindowsWindow` / `LinuxWindow` are concrete implementations using GLFW.
- Events are value types dispatched through a callback. No dynamic allocation per event.
- Input polling (for held keys/buttons) is separate from event dispatch.

### Event System Architecture
Event (base, non-copyable)
├── WindowResizeEvent, WindowCloseEvent
├── KeyPressedEvent, KeyReleasedEvent, KeyTypedEvent
├── MouseMovedEvent, MouseScrolledEvent
└── MouseButtonPressedEvent, MouseButtonReleasedEvent
EventDispatcher  ← Matches event type and calls typed handler

**Dispatch pattern:**
```cpp
EventDispatcher dispatcher(event);
dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
```

### Implementation Steps

1. **Event base class** with `EventType`, `EventCategory` bitmask (Input, Keyboard, Mouse, Application), `Handled` flag.
2. **Concrete event classes** — each stores relevant data (key code, mouse position, etc.).
3. **EventDispatcher** — templated `Dispatch<T>()` that checks type and invokes callback.
4. **Window interface** (`IWindow`)
   - `Create(WindowProps)` static factory
   - `OnUpdate()` — polls GLFW events
   - `SetEventCallback(fn)` — stores callback invoked from GLFW callbacks
   - `SetVSync(bool)`, `GetWidth/Height()`, `GetNativeWindow()`
5. **GLFW Window implementation**
   - Set GLFW callbacks (key, mouse button, scroll, cursor, window resize, window close) that construct the appropriate event and call the stored callback.
6. **Input class (polling)**
   - Static `IsKeyPressed(KeyCode)`, `IsMouseButtonPressed(MouseCode)`, `GetMousePosition()` using `glfwGetKey` etc.
   - Define `KeyCode` and `MouseCode` enums mapping to GLFW codes.

### Milestone
- Window opens, events dispatch correctly to layers, keyboard/mouse polling works.

---

## 4. Renderer

### Goal
Build a high-quality, batched 2D renderer and a forward-rendered 3D renderer on top of the abstraction layer (Section 5). Integrate ImGui for the editor UI.

### Renderer Architecture
Renderer (high-level API, scene submission)
├── Renderer2D          ← Batched quads, circles, lines, text
├── Renderer3D          ← Mesh submission, lighting, forward pass
└── RenderCommand       ← Low-level draw calls, state changes
└── RendererAPI (abstract) → OpenGLRendererAPI
### 4.1 Renderer2D — Batched Sprite/Shape Renderer

**Data flow:**
1. `Renderer2D::BeginScene(camera)` — uploads view-projection UBO
2. User calls `DrawQuad(transform, texture, tint)`, `DrawCircle(...)`, `DrawLine(...)`
3. Internally fills a CPU-side vertex buffer (batch)
4. When batch is full or `EndScene()` is called — flush: upload to GPU, bind texture array/atlas, draw call
5. `Renderer2D::EndScene()` — flushes remaining geometry, resets batch state

**Implementation Steps:**
1. Quad batch: pre-allocated `QuadVertex[]` array (position, color, UV, texIndex, entityID)
2. Texture slot system: bind up to N textures per draw call (query `GL_MAX_TEXTURE_IMAGE_UNITS`)
3. Circle and line batches (separate shaders)
4. Renderer2D statistics: draw calls, quad count per frame (display in ImGui debug panel)
5. Camera abstraction: `OrthographicCamera`, `OrthographicCameraController`

### 4.2 Renderer3D — Forward Renderer

**Implementation Steps:**
1. Mesh loading pipeline: vertex/index buffer upload, VAO setup
2. Material system: PBR-ready material struct (albedo, metallic, roughness, normal map slots)
3. Lighting UBO: directional light, point lights array
4. Render passes: geometry pass → post-processing pass (simple fullscreen quad)
5. Skybox / environment cubemap
6. Camera: `PerspectiveCamera`, `EditorCamera` (arcball, fly-through)

### 4.3 Framebuffer

- `Framebuffer` abstraction with color attachment(s) + depth/stencil
- Resizable — recreate on window resize
- Used by the editor viewport (render scene to texture, display in ImGui `Image()`)
- Mouse picking: second color attachment stores entity ID (integer texture), read pixel on click

### 4.4 Shader System

1. `Shader` class: compile vert/frag (+ optional geom/compute), link, cache uniforms
2. `ShaderLibrary`: load from file, cache by name, hot-reload on file change (later milestone)
3. GLSL `#include` preprocessor directive support for shared utility code
4. Uniform Buffer Objects (UBOs) for camera matrices and per-frame data (avoid per-draw uniform uploads)

### Milestone
- Editor viewport renders a 2D scene (sprites, tilemaps) and a 3D scene (lit mesh) into a framebuffer displayed in an ImGui panel.

---

## 5. Render API Abstraction

### Goal
Isolate all OpenGL calls behind abstract interfaces so that a Vulkan or DirectX 12 backend could be slotted in later.

### Design

All GPU resource types are abstract base classes. Factory functions choose the concrete implementation based on the active `RendererAPI`.

RendererAPI (enum: None, OpenGL, Vulkan, DirectX12)
VertexBuffer (interface)  →  OpenGLVertexBuffer
IndexBuffer  (interface)  →  OpenGLIndexBuffer
VertexArray  (interface)  →  OpenGLVertexArray
Shader       (interface)  →  OpenGLShader
Texture2D    (interface)  →  OpenGLTexture2D
Texture3D    (interface)  →  OpenGLTexture3D
Cubemap      (interface)  →  OpenGLCubemap
Framebuffer  (interface)  →  OpenGLFramebuffer
UniformBuffer(interface)  →  OpenGLUniformBuffer

### Implementation Steps

1. **Buffer Layout system**
   - `BufferElement` — name, `ShaderDataType` (Float, Float2 ... Mat4, Int, Bool), normalized flag, offset
   - `BufferLayout` — list of elements, auto-computes stride and offsets
2. **VertexBuffer** — `Create(size)` (dynamic) and `Create(data, size)` (static); `SetLayout()`; `SetData()` for dynamic update
3. **IndexBuffer** — `Create(indices, count)`
4. **VertexArray** — `AddVertexBuffer()`; `SetIndexBuffer()`; `Bind()`/`Unbind()`
5. **Shader** — per above (Section 4.4)
6. **Texture** — `Create(spec)` where spec carries width, height, format (`RGBA8`, `RED_INTEGER`, `DEPTH24STENCIL8`), filter, wrap mode; `SetData(void*, size)` for CPU upload
7. **Framebuffer** — `FramebufferSpec` with attachments list; `Resize()`; `ReadPixel(attachment, x, y)` for picking
8. **RenderCommand** — static wrappers: `Clear()`, `SetClearColor()`, `DrawIndexed()`, `DrawIndexedInstanced()`, `SetLineWidth()`
9. **RendererAPI** — abstract with the above as pure virtual; `RendererAPI::Create()` factory

### Milestone
- Entire rendering pipeline operates through abstract interfaces; swapping OpenGL for another backend requires only changing the factory return and providing new concrete classes.

---

## 6. Debugging Support

### Goal
Provide first-class logging, runtime assertions, profiling, and an in-engine debug overlay so problems surface immediately during development.

### 6.1 Logging — spdlog

1. Wrap spdlog in an `Log` class with two loggers: **Core** (engine-internal) and **Client** (game code).
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

### 6.2 Assertions

```cpp
#define ELYSIUM_ASSERT(x, ...) if (!(x)) { ELYSIUM_ERROR(__VA_ARGS__); __debugbreak(); }
#define ELYSIUM_CORE_ASSERT(x, ...) ...
```
- Disabled in `ELYSIUM_DIST` (distribution) builds.

### 6.3 Instrumentation Profiler

1. `Timer` class using `std::chrono::high_resolution_clock`
2. `Instrumentor` — singleton, writes Chrome Tracing JSON (`chrome://tracing`) on session end
3. `InstrumentationTimer` — RAII scope timer, reports to `Instrumentor`
4. Macros:
```cpp
   ELYSIUM_PROFILE_BEGIN("Session", "profile.json")
   ELYSIUM_PROFILE_END()
   ELYSIUM_PROFILE_SCOPE("name")
   ELYSIUM_PROFILE_FUNCTION()   // uses __FUNCSIG__ / __PRETTY_FUNCTION__
```
5. Strip to no-ops outside of `ELYSIUM_PROFILE` build config.

### 6.4 ImGui Debug Panels

- **Renderer Stats panel**: draw calls, vertex count, frame time, GPU memory (via `GL_NVX_gpu_memory_info` or `GL_ATI_meminfo`)
- **Entity inspector panel**: selected entity's components, editable fields
- **Memory panel**: allocator stats (Section 8)
- **Console panel**: display log output inside ImGui
- **Physics debugger**: visualize colliders, broadphase AABBs, contact points via `Renderer2D` lines (togglable)

### 6.5 OpenGL Debug Callback

```cpp
glEnable(GL_DEBUG_OUTPUT);
glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
glDebugMessageCallback(OpenGLMessageCallback, nullptr);
glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
```
- Route OpenGL errors through `ELYSIUM_CORE_ERROR`.

### Milestone
- All engine paths log appropriately; profiling sessions produce loadable JSON; OpenGL errors surface immediately.

---

## 7. Scripting Language

### Goal
Embed Lua (via Sol2) as the primary scripting language, allowing gameplay logic to be written without recompiling the engine.

### Design

- Lua scripts are attached to entities as a `ScriptComponent`.
- The engine binds engine types (Entity, Input, Vec2/3, Transform, etc.) to Lua.
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
   - Bind `Entity`, `TransformComponent`, `RigidBodyComponent`, `Input`, `Log`, `Vec2`, `Vec3`, `Mat4`
   - Bind `Scene` — `CreateEntity()`, `DestroyEntity()`, `FindEntityByName()`

3. **ScriptComponent**
   - Stores script file path and a `sol::table` (the script's `self` instance).
   - `ScriptingEngine::OnCreateEntity(entity)` — loads the Lua file, instantiates a table, calls `OnCreate`.
   - `ScriptingEngine::OnUpdateEntity(entity, dt)` — calls `OnUpdate(dt)` per frame.

4. **Hot reloading**
   - Watch script files for modifications (using platform file watching or polling).
   - On change: call `OnDestroy`, re-execute file, call `OnCreate`.

5. **Error handling**
   - Wrap all Lua calls in `sol::protected_function` and route errors to the engine logger.

6. **Script editor integration (ImGui)**
   - File browser panel to assign scripts to entities.
   - Basic in-editor Lua syntax highlighting (via `ImGui::InputTextMultiline` + a simple lexer, or embed a text editor library like ImGuiColorTextEdit).

### Future Extension
- C# scripting via Mono (more complex, deferred to a later milestone).

### Milestone
- An entity with a `ScriptComponent` runs Lua that reads input and modifies a `RigidBodyComponent` each frame.

---

## 8. Memory Systems

### Goal
Replace uncontrolled heap allocation with purpose-built allocators that improve cache efficiency, reduce fragmentation, enable leak detection, and give insight into memory usage.

### 8.1 Allocator Types

| Allocator | Use Case |
|---|---|
| Linear / Arena | Per-frame scratch space, temporary parsing buffers |
| Stack | Nested scope allocations, undo/redo state |
| Pool | Fixed-size objects: entities, components, events, particles |
| Free-List (General) | Variable-size long-lived allocations, fallback |
| TLSF (optional) | Real-time safe general allocator with bounded fragmentation |

### Implementation Steps

1. **Base `IAllocator` interface**: `Allocate(size, alignment)`, `Deallocate(ptr, size)`, `Reset()` (where applicable)
2. **LinearAllocator**: offset pointer into a pre-allocated block; `Reset()` returns to zero; no per-item free.
3. **StackAllocator**: like linear but with a marker stack to pop frames.
4. **PoolAllocator**: slab of N fixed-size blocks; free list of available slots; O(1) alloc/free; template-typed variant `PoolAllocator<T, N>`.
5. **FreeListAllocator**: doubly-linked free block list with header metadata; first-fit or best-fit policy; coalescing on free.
6. **MemoryManager singleton**
   - Owns a large backing block (e.g. 512 MB virtual reservation).
   - Hands out sub-arenas to each subsystem.
   - Tracks per-subsystem usage, peak usage, allocation count.
7. **Overriding `new`/`delete`** (optional but powerful):
   - Provide a global `operator new` that routes through the engine allocator.
   - Tag allocations with source file/line in debug builds.
8. **Leak detection**
   - In debug builds maintain an `std::unordered_map<void*, AllocationInfo>` tracking every live allocation.
   - On shutdown, log all live allocations as leaks with file/line/size.
9. **ImGui memory panel**: bar graph of allocator usage vs capacity per subsystem.

### Milestone
- All engine subsystems allocate through named allocators; per-frame scratch allocations leave zero fragmentation; leaks are reported on shutdown.

---

## 9. Entity Component System

### Goal
Replace any ad-hoc game object hierarchy with a fast, cache-friendly ECS using EnTT as the backbone, wrapped in an ergonomic engine-level API.

### Design

- `Scene` owns an `entt::registry`.
- `Entity` is a thin wrapper around `entt::entity` + a pointer to its owning `Scene`.
- Components are plain data structs (no virtual functions).
- Systems are free functions or classes that query the registry.

### Built-in Components

| Component | Data |
|---|---|
| `TagComponent` | `std::string Name` |
| `TransformComponent` | `Vec3 Position, Rotation, Scale`; `GetTransform() -> Mat4` |
| `SpriteRendererComponent` | `Texture2DRef`, `Vec4 Color`, `float TilingFactor` |
| `MeshComponent` | `MeshRef`, `MaterialRef` |
| `CameraComponent` | `SceneCamera`, `bool Primary`, `bool FixedAspect` |
| `ScriptComponent` | `std::string ScriptPath`, `sol::table Instance` |
| `RigidBodyComponent` | `BodyType (Static/Kinematic/Dynamic)`, `Vec2 Velocity`, `float Mass` |
| `BoxCollider2DComponent` | `Vec2 Offset, Size`, `float Density, Friction, Restitution` |
| `CircleCollider2DComponent` | `Vec2 Offset`, `float Radius`, physics material params |
| `PointLightComponent` | `Vec3 Color`, `float Intensity`, `float Radius` |
| `DirectionalLightComponent`| `Vec3 Direction, Color`, `float Intensity` |
| `AudioSourceComponent` | audio clip ref, volume, pitch, loop |

### Implementation Steps

1. **Scene class**
   - `CreateEntity(name)` → wraps `registry.create()` + adds `TagComponent` + `TransformComponent`
   - `DestroyEntity(entity)` → `registry.destroy()`
   - `OnUpdate(Timestep)` — run all systems
   - `OnRender()` — query render components and submit to Renderer
   - `OnViewportResize(w, h)` — update camera aspect ratios
2. **Entity class**
   - Templated `AddComponent<T>(args...)`, `GetComponent<T>()`, `HasComponent<T>()`, `RemoveComponent<T>()`
3. **Systems** (standalone functions iterating registry views)
   - `ScriptSystem::OnUpdate(registry, dt)`
   - `PhysicsSystem::OnUpdate(registry, dt)` — feeds physics engine
   - `RenderSystem::OnRender(registry)` — submits drawcalls
   - `AudioSystem::OnUpdate(registry, dt)`
4. **Scene serialization / deserialization** (YAML via yaml-cpp)
   - `SceneSerializer::Serialize(scene, filepath)` — write all entities and components to YAML
   - `SceneSerializer::Deserialize(filepath)` — reconstruct scene from YAML
5. **Scene hierarchy** (parent/child relationships)
   - `RelationshipComponent`: parent entity handle, first child, next sibling (doubly-linked list pattern)
   - Transform system: compute world transform from local + parent world transform
6. **Prefabs**
   - Serialize a single entity subtree to a YAML prefab file.
   - `Scene::InstantiatePrefab(filepath, parent)` — deserialize and attach.

### Editor Integration
- Hierarchy panel: tree view of all entities, drag to reparent
- Inspector panel: display and edit all components via ImGui; `AddComponent` dropdown
- Scene panel: select entities by clicking in the viewport (using framebuffer entity ID picking)

### Milestone
- A scene with 10 000 entities with transform + sprite components renders and updates at 60 fps. Save and load round-trip produces identical scenes.

---

## 10. Physics Engine

### Goal
Evolve the existing 2D physics engine from a simple collision system into a robust, feature-complete 2D physics simulation, and lay groundwork for optional 3D physics integration.

### 10.1 Architecture

PhysicsEngine2D (singleton or subsystem)
├── PhysicsWorld2D          ← owns all bodies, broadphase, solver
│   ├── BroadPhase          ← AABB tree / sweep-and-prune
│   ├── NarrowPhase         ← SAT / GJK collision detection
│   ├── ConstraintSolver    ← Sequential impulse (Erin Catto method)
│   └── RigidBody2D[]
└── PhysicsDebugDraw        ← renders colliders via Renderer2D lines

### 10.2 Core Features to Build

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
- Route to ScriptComponent Lua callbacks

**Integration & Timestep**
- Fixed timestep integration (default 1/60 s) with accumulator
- Semi-implicit Euler integration
- Substep support for tunnelling prevention (fast-moving objects)

### 10.3 Integration with ECS

- `PhysicsSystem` reads `RigidBodyComponent` + `BoxCollider2DComponent` / `CircleCollider2DComponent` from registry
- On scene start: create physics bodies and register in `PhysicsWorld2D`
- On scene stop: destroy all physics state
- Each frame: `PhysicsWorld2D::Step(fixedDt)` → write back positions/rotations to `TransformComponent`

### 10.4 Optional 3D Physics

- Integrate **Jolt Physics** or **Bullet Physics** as the 3D backend
- Expose `RigidBody3DComponent`, `BoxCollider3DComponent`, `SphereCollider3DComponent`
- Same ECS pattern as 2D

### 10.5 Physics Debugger

- Toggle-able via ImGui panel
- Render all collider outlines, velocity vectors, contact normals using `Renderer2D` lines
- Display body count, active island count, contact count

### Milestone
- A scene with 500 dynamic boxes stacked and falling simulates at 60 fps with stable stacking, correct restitution, and triggers firing Lua callbacks.

---

## 11. File I/O & Virtual File System

### Goal
Provide a unified, platform-agnostic file access layer that abstracts over real disk paths, supports asset packaging (PAK files), and enables hot-reloading of assets.

### 11.1 Virtual File System (VFS) Design
VFS
├── MountPoint map: { "assets://", "shaders://", "scripts://" }
│   Each maps to one or more IFileProvider:
│   ├── DiskFileProvider     ← reads from a real directory
│   └── PackFileProvider     ← reads from a PAK archive
└── FileHandle

- Paths in engine code always use virtual paths: `"assets://textures/player.png"`
- VFS resolves them through the mount table at runtime.
- Multiple providers can be stacked on one mount point (archive overlays real dir, or vice versa).

### 11.2 Implementation Steps

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
   - `VFS::ReadAsync(path, callback)` — submits to a thread pool, calls callback on completion
   - Used by the asset manager to load textures, meshes, audio in the background

6. **PAK Archive format**
   - Simple custom format: header + file table (name hash → offset + size) + compressed data (zstd or zlib)
   - `PackBuilder` tool: pack a directory tree into a `.pak` file
   - `PackFileProvider`: memory-maps the PAK, serves files from it

7. **Asset Manager** (built on VFS)
   - `AssetManager::Load<Texture2D>("assets://textures/player.png")` → returns `Ref<Texture2D>`
   - Caches by path; returns existing handle if already loaded
   - Reference-counted: unloads when last `Ref` is dropped
   - `AssetHandle` type (UUID) for serialisation in scene files (instead of storing raw paths)

8. **File Watcher (Hot Reload)**
   - Background thread polls file modification times (or uses OS inotify / ReadDirectoryChangesW)
   - On change: notify registered watchers
   - Shader hot-reload, texture hot-reload, Lua script hot-reload hook in here

### 11.3 Serialisation Formats

| Data | Format |
|---|---|
| Scenes, prefabs | YAML (yaml-cpp) |
| Engine config | INI or YAML |
| Binary meshes | Custom `.emesh` (positions, normals, UVs, indices, material refs) |
| Binary textures | `.ktx2` (GPU-compressed, mip-mapped) via basisu / toktx |
| PAK archives | Custom `.elypak` |

### Milestone
- All engine assets are loaded through virtual paths; a PAK archive can replace the loose file directory transparently; shader file changes hot-reload without restarting.

---

## Development Phases & Suggested Order

### **Phase 1 — Foundation (Months 1–2)**
Entry Point → Application Layer → Window/Events → Logging & Assertions → Basic OpenGL Renderer (clear, triangle, shader)

### **Phase 2 — Core Renderer (Months 3–4)**
Render API Abstraction → Shader system → Buffers & VAOs → Texture2D → Framebuffer → 2D Batch Renderer → ImGui integration → Editor viewport

### **Phase 3 — Editor Shell (Month 5)**
ImGui docking layout → Hierarchy panel → Inspector panel → Scene serialization (YAML) → Asset drag-and-drop

### **Phase 4 — ECS & Scene (Month 6)**
EnTT integration → Built-in components → Scene systems → Prefab system → Entity picking

### **Phase 5 — Physics & Scripting (Months 7–8)**
Physics engine rewrite (broadphase, narrowphase, solver) → ECS physics integration → Lua + Sol2 embedding → Script component → Physics debug draw

### **Phase 6 — Memory & File Systems (Month 9)**
Allocator library → MemoryManager → VFS + DiskProvider → AssetManager + caching → File watcher + hot-reload

### **Phase 7 — 3D Renderer & Lighting (Months 10–11)**
3D mesh pipeline → Forward lighting (Phong/PBR) → Shadow mapping → Skybox → EditorCamera → Post-processing stack

### **Phase 8 — Polish & Tooling (Month 12+)**
PAK packaging → Audio system → Profiler UI → Distribution build pipeline → Documentation

---

*Elysium Engine — Built from scratch, built to last.*
