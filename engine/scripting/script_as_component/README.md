# Elysium Engine — Script as Component Subsystem

The `engine/scripting/script_as_component` module provides a component-based scripting architecture allowing GameObjects to be dynamically augmented with custom behaviors at runtime or via the Dear ImGui Editor.

---

## Directory Contents

| Header | Source | Description |
|---|---|---|
| [`ScriptComponent.hpp`](include/ScriptComponent.hpp) | [`ScriptComponent.cpp`](src/ScriptComponent.cpp) | Component holding an instantiated script, forwarding lifecycle events, and managing bindings. |
| [`ScriptRegistry.hpp`](include/ScriptRegistry.hpp) | [`ScriptRegistry.cpp`](src/ScriptRegistry.cpp) | Factory registry for registering and dynamically instantiating scripts by string identifier. |

---

## Core Concepts

### Script Lifecycle Execution
When a `ScriptComponent` is attached to a `GameObject`, it orchestrates the lifecycle of the underlying `Script`:
1. **Binding**: `BindScript(name)` instantiates the registered script class, binds the safe `Entity` handle, and immediately calls `OnCreate()`.
2. **Update**: Each simulation frame, `Scene::Update()` updates all entity components, calling `ScriptComponent::Update(dt)`, which forwards to `Script::OnUpdate(dt)`.
3. **Unbinding**: When a script is removed or replaced, `UnbindScript()` invokes `OnDestroy()` and cleans up entity handles to prevent memory leaks.

### Registry Pattern
Because C++ lacks native runtime reflection, `ScriptRegistry` maps string class names to factory lambdas. This enables:
- Dynamic script attachment by name from scene files or network packets.
- Automatic population of script dropdown menus inside the editor inspector.
- Decoupling of entity spawning from concrete gameplay script classes.
