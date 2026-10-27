# Elysium Engine — Scripting & Wrapper Subsystem

The `engine/scripting` module provides a high-level, Godot-style C++ wrapper API designed for rapid gameplay scripting. It abstracts internal engine plumbing—pointer management, collider calculations, matrix synchronizations, and unit conversions—behind safe value-type handles and straightforward lifecycle hooks.

---

## Directory Contents

| Header | Source | Description |
|---|---|---|
| [`ScriptingAPI.hpp`](include/ScriptingAPI.hpp) | — | Umbrella header including all scripting types, handles, and helper functions. |
| [`Entity.hpp`](include/Entity.hpp) | [`Entity.cpp`](src/Entity.cpp) | Safe value-type handle (`std::weak_ptr`) wrapping GameObjects, transforms, physics, and materials. |
| [`Script.hpp`](include/Script.hpp) | — | Component base class providing `OnCreate()`, `OnUpdate(dt)`, and `OnDestroy()` lifecycle hooks. |
| [`ScriptingWorld.hpp`](include/ScriptingWorld.hpp) | [`ScriptingWorld.cpp`](src/ScriptingWorld.cpp) | One-line spawning helpers (`SpawnBox`, `SpawnCircle`), entity queries, and world bounds/gravity settings. |
| [`ScriptingInput.hpp`](include/ScriptingInput.hpp) | [`ScriptingInput.cpp`](src/ScriptingInput.cpp) | High-level keyboard and mouse polling with keycode and string name overloads. |
| [`ScriptEngine.hpp`](include/ScriptEngine.hpp) | [`ScriptEngine.cpp`](src/ScriptEngine.cpp) | Central subsystem coordinating scene transitions, script lifecycle execution, and future language bindings. |
| [`ScriptTypes.hpp`](include/ScriptTypes.hpp) | — | Scripting type aliases for 2D vectors (`Vector2` / `Vec2`, defined in `CoreMath.hpp`) and color presets (`ColorRGBA`). |

---

## Core Concepts

### Entity Handle Pattern
Direct interaction with `GameObject*` and `RigidBody*` poses safety risks (dangling pointers, manual synchronization). The `Entity` class wraps an internal `std::weak_ptr<GameObject>`:
- **Lifetime Safety**: Method calls on destroyed entities fail without segmentation faults.
- **Automatic Physics Sync**: Updating transform properties (`SetPosition`, `Move`, `SetRotation`) automatically updates internal physics matrices and centroid states via `SyncTransform()`.

### Script Lifecycle
Custom behaviors derive from `Elysium::Script`:
- `OnCreate()`: Invoked once when the script is attached to an entity.
- `OnUpdate(float dt)`: Invoked once per simulation frame with delta time in seconds.
- `OnDestroy()`: Invoked when the entity or script is detached/destroyed.
- `entity`: Direct member pointer to the owning `Entity` handle.

### Spawning & World Abstraction
World functions (`SpawnBox`, `SpawnCircle`, `SpawnStaticBox`, `SpawnStaticCircle`) automate multi-step setup:
1. Instantiating a `GameObject`.
2. Calculating collider half-extents or radii.
3. Attaching a `RigidBody` and configuring mass/restitution/friction.
4. Attaching a `SpriteRenderer` with color tinting.
5. Registering the entity with the active `Scene`.

### Input & Coordinate Translation
- **Input Polling**: Queries keys and buttons by enum (`Key::Space`) or string (`"SPACE"`).
- **Coordinate Conversion**: Automatically converts raw window pixel coordinates to world physics meters based on the engine's 50 px/m scale factor.


