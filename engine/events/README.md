# Elysium Engine — Event System

The `engine/events` module implements a type-safe, zero-dynamic-allocation event pipeline for the Elysium Game Engine.

---

## Directory Contents

| Header | Description |
|---|---|
| [`Event.hpp`](Event.hpp) | Base `Event` class, category bitmasks, and templated `EventDispatcher`. |
| [`ApplicationEvent.hpp`](ApplicationEvent.hpp) | Window events (`WindowCloseEvent`, `WindowResizeEvent`). |
| [`KeyEvent.hpp`](KeyEvent.hpp) | Keyboard events (`KeyPressedEvent`, `KeyReleasedEvent`, `KeyTypedEvent`). |
| [`MouseEvent.hpp`](MouseEvent.hpp) | Mouse events (`MouseMovedEvent`, `MouseScrolledEvent`, `MouseButtonPressedEvent`, `MouseButtonReleasedEvent`). |

---

## Architecture & Design

### Event Categories
Events belong to one or more bitmask categories:
- `EventCategoryApplication`: Window resize, close, focus.
- `EventCategoryInput`: Any user input event.
- `EventCategoryKeyboard`: Key press, release, character type.
- `EventCategoryMouse`: Cursor motion, scrolling.
- `EventCategoryMouseButton`: Mouse button clicks.

### EventDispatcher
The `EventDispatcher` inspects an event's type and calls the matching typed callback function:

```cpp
void OnEvent(Elysium::Event& event) {
  Elysium::EventDispatcher dispatcher(event);

  dispatcher.Dispatch<Elysium::KeyPressedEvent>([this](Elysium::KeyPressedEvent& e) {
    if (e.GetKeyCode() == Elysium::Key::Space) {
      // Handle jump or toggle
      return true; // Return true to mark event as handled
    }
    return false;
  });

  dispatcher.Dispatch<Elysium::WindowResizeEvent>([this](Elysium::WindowResizeEvent& e) {
    // Handle viewport or projection resize
    return false;
  });
}
```

### Dispatch Flow
1. **OS Polling**: `Window::OnUpdate()` polls `sf::RenderWindow` events.
2. **ImGui Processing**: Raw `sf::Event` is sent to `ImGui::SFML::ProcessEvent()` first.
3. **Translation**: The `sf::Event` is translated into an Elysium `Event` value type on the stack.
4. **Distribution**: `Application::OnEvent()` dispatches the event down the `LayerStack` in **reverse order** (overlays get the event first).
5. **Propagation Stop**: If any layer sets `event.Handled = true`, dispatching halts immediately.
