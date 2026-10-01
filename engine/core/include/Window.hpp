#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "Core.hpp"
#include "Event.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <functional>
#include <memory>
#include <string>

namespace Elysium {

struct WindowProps {
  std::string Title;
  unsigned int Width;
  unsigned int Height;

  WindowProps(const std::string &title = "Elysium Engine",
              unsigned int width = 1280, unsigned int height = 720)
      : Title(title), Width(width), Height(height) {}
};

class ELYSIUM_API Window {
public:
  using EventCallbackFn = std::function<void(Event &)>;

  Window(const WindowProps &props = WindowProps());
  ~Window();

  void OnUpdate();
  void Display();
  void Clear(const sf::Color &color = sf::Color(20, 20, 25));

  unsigned int GetWidth() const { return m_Data.Width; }
  unsigned int GetHeight() const { return m_Data.Height; }

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

  sf::RenderWindow m_Window;

  struct WindowData {
    std::string Title;
    unsigned int Width = 1280;
    unsigned int Height = 720;
    bool VSync = true;
    EventCallbackFn EventCallback;
  };

  WindowData m_Data;
};

} // namespace Elysium

#endif // WINDOW_HPP
