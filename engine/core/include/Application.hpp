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

class AudioEngine;

struct ApplicationCommandLineArgs {
  int Count = 0;
  char **Args = nullptr;

  const char *operator[](int index) const {
    return (index >= 0 && index < Count && Args) ? Args[index] : nullptr;
  }
};

class ELYSIUM_API Application {
public:
  Application(const WindowProps &windowProps,
              ApplicationCommandLineArgs args = {});
  Application(const std::string &name = "Elysium Application",
              ApplicationCommandLineArgs args = {});
  virtual ~Application();

  void Run();
  void OnEvent(Event &e);

  void PushLayer(Layer *layer);
  void PushOverlay(Layer *overlay);

  Window &GetWindow() { return *m_Window; }
  const Window &GetWindow() const { return *m_Window; }

  AudioEngine &GetAudioEngine() { return *m_AudioEngine; }
  const AudioEngine &GetAudioEngine() const { return *m_AudioEngine; }

  const ApplicationCommandLineArgs &GetCommandLineArgs() const {
    return m_CommandLineArgs;
  }

  void Close() { m_Running = false; }

  static Application &Get() { return *s_Instance; }

private:
  bool OnWindowClose(WindowCloseEvent &e);
  bool OnWindowResize(WindowResizeEvent &e);

  ApplicationCommandLineArgs m_CommandLineArgs;
  std::unique_ptr<Window> m_Window;
  std::unique_ptr<AudioEngine> m_AudioEngine;
  LayerStack m_LayerStack;
  bool m_Running = true;
  bool m_Minimized = false;
  sf::Clock m_Clock;

  static Application *s_Instance;
};

// Client factory function
Application *CreateApplication(ApplicationCommandLineArgs args);

} // namespace Elysium

#endif // APPLICATION_HPP
