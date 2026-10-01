#include "Application.hpp"
#include "Log.h"

namespace Elysium {

Application *Application::s_Instance = nullptr;

Application::Application(const std::string &name) {
  if (s_Instance) {
    ELYSIUM_CORE_ERROR("Application already exists!");
  }
  s_Instance = this;

  m_Window = std::make_unique<Window>(WindowProps(name));
  m_Window->SetEventCallback([this](Event &e) { OnEvent(e); });
}

Application::~Application() { s_Instance = nullptr; }

void Application::PushLayer(Layer *layer) { m_LayerStack.PushLayer(layer); }

void Application::PushOverlay(Layer *overlay) {
  m_LayerStack.PushOverlay(overlay);
}

void Application::OnEvent(Event &e) {
  EventDispatcher dispatcher(e);
  dispatcher.Dispatch<WindowCloseEvent>(
      [this](WindowCloseEvent &event) { return OnWindowClose(event); });
  dispatcher.Dispatch<WindowResizeEvent>(
      [this](WindowResizeEvent &event) { return OnWindowResize(event); });

  for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it) {
    if (e.Handled)
      break;
    (*it)->OnEvent(e);
  }
}

bool Application::OnWindowClose(WindowCloseEvent &e) {
  (void)e;
  m_Running = false;
  return true;
}

bool Application::OnWindowResize(WindowResizeEvent &e) {
  if (e.GetWidth() == 0 || e.GetHeight() == 0) {
    m_Minimized = true;
    return false;
  }
  m_Minimized = false;
  return false;
}

void Application::Run() {
  while (m_Running && m_Window->IsOpen()) {
    float time = m_Clock.restart().asSeconds();
    Timestep timestep = (time > 0.1f) ? 0.1f : time;

    m_Window->OnUpdate();

    if (!m_Minimized) {
      for (Layer *layer : m_LayerStack) {
        layer->OnUpdate(timestep);
      }

      m_Window->Clear(sf::Color(22, 22, 26));

      for (Layer *layer : m_LayerStack) {
        layer->OnImGuiRender();
      }

      m_Window->Display();
    }
  }
}

} // namespace Elysium
