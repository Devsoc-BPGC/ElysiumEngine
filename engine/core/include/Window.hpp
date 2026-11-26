#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "Core.hpp"
#include "Event.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Cursor.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace Elysium {

struct WindowProps {
  std::string Title;
  unsigned int Width;
  unsigned int Height;
  bool Resizable;

  WindowProps(const std::string &title = "Elysium Engine",
              unsigned int width = 1280, unsigned int height = 720,
              bool resizable = true)
      : Title(title), Width(width), Height(height), Resizable(resizable) {}
};

class ELYSIUM_API Window {
public:
  using EventCallbackFn = std::function<void(Event &)>;

  Window(const WindowProps &props = WindowProps());
  ~Window();

  void OnUpdate();
  void Display();
  void Clear(const sf::Color &color = sf::Color(20, 20, 25));

  void Reconfigure(unsigned int width, unsigned int height,
                   const std::string &title, bool resizable);
  void ResetMouseCursor();

  unsigned int GetWidth() const { return m_Data.Width; }
  unsigned int GetHeight() const { return m_Data.Height; }
  bool IsResizable() const { return m_Data.Resizable; }

  void SetEventCallback(const EventCallbackFn &callback) {
    m_Data.EventCallback = callback;
  }
  void SetVSync(bool enabled);
  bool IsVSync() const { return m_Data.VSync; }

  sf::RenderWindow &GetNativeWindow() { return m_Window; }
  const sf::RenderWindow &GetNativeWindow() const { return m_Window; }

  bool IsOpen() const { return m_Window.isOpen(); }
  void Close() { m_Window.close(); }

private:
  void Init(const WindowProps &props);
  void Shutdown();
  void CenterOnDesktop();

  // Keep default cursor alive longer than m_Window so X11 never sees a freed cursor
  std::optional<sf::Cursor> m_DefaultCursor;
  sf::RenderWindow m_Window;

  struct WindowData {
    std::string Title;
    unsigned int Width = 1280;
    unsigned int Height = 720;
    bool Resizable = true;
    bool VSync = true;
    EventCallbackFn EventCallback;
  };

  WindowData m_Data;
};

} // namespace Elysium

#endif // WINDOW_HPP
