#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "ApplicationEvent.hpp"
#include "Core.hpp"
#include "Event.hpp"
#include "LayerStack.hpp"
#include "Timestep.hpp"
#include "Window.hpp"
#include <SFML/System/Clock.hpp>
#include <memory>
#include <string>

namespace Elysium {

class ELYSIUM_API Application {
public:
  Application(const std::string &name = "Elysium Application");
  virtual ~Application();

  void Run();
  void OnEvent(Event &e);

  void PushLayer(Layer *layer);
  void PushOverlay(Layer *overlay);

  Window &GetWindow() { return *m_Window; }
  const Window &GetWindow() const { return *m_Window; }

  void Close() { m_Running = false; }

  static Application &Get() { return *s_Instance; }

private:
  bool OnWindowClose(WindowCloseEvent &e);
  bool OnWindowResize(WindowResizeEvent &e);

  std::unique_ptr<Window> m_Window;
  LayerStack m_LayerStack;
  bool m_Running = true;
  bool m_Minimized = false;
  sf::Clock m_Clock;

  static Application *s_Instance;
};

// Client factory function
Application *CreateApplication();

} // namespace Elysium

#endif // APPLICATION_HPP
