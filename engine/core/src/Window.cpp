#include "elypch.hpp"
#include "Window.hpp"
#include "ApplicationEvent.hpp"
#include "Input.hpp"
#include "KeyEvent.hpp"
#include "MouseEvent.hpp"
#include <SFML/Window/Event.hpp>
#include <imgui-SFML.h>

namespace Elysium {

Window::Window(const WindowProps &props) { Init(props); }

Window::~Window() { Shutdown(); }

void Window::Init(const WindowProps &props) {
  m_Data.Title = props.Title;
  m_Data.Width = props.Width;
  m_Data.Height = props.Height;
  m_Data.Resizable = props.Resizable;

  ELYSIUM_CORE_INFO("Creating window {} ({}x{}, {})", props.Title, props.Width,
                    props.Height, props.Resizable ? "resizable" : "floating");

  m_DefaultCursor = sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow);

  const std::uint32_t style =
      props.Resizable ? sf::Style::Default
                      : (sf::Style::Titlebar | sf::Style::Close);
  m_Window.create(sf::VideoMode({props.Width, props.Height}), props.Title,
                  style);
  ResetMouseCursor();
  if (!props.Resizable) {
    CenterOnDesktop();
  }
  SetVSync(true);
}

void Window::ResetMouseCursor() {
  if (m_Window.isOpen() && m_DefaultCursor.has_value()) {
    m_Window.setMouseCursorVisible(true);
    m_Window.setMouseCursor(*m_DefaultCursor);
  }
}

void Window::Reconfigure(unsigned int width, unsigned int height,
                         const std::string &title, bool resizable) {
  m_Data.Title = title;
  m_Data.Width = width;
  m_Data.Height = height;
  m_Data.Resizable = resizable;

  ELYSIUM_CORE_INFO("Reconfiguring window {} ({}x{}, {})", title, width,
                    height, resizable ? "resizable" : "floating");

  // Detach any temporary external cursor (e.g. ImGui-SFML's freed cursors) before create()
  ResetMouseCursor();

  const std::uint32_t style =
      resizable ? sf::Style::Default
                : (sf::Style::Titlebar | sf::Style::Close);
  m_Window.create(sf::VideoMode({width, height}), title, style);
  ResetMouseCursor();
  if (!resizable) {
    CenterOnDesktop();
  }
  SetVSync(m_Data.VSync);
}

void Window::CenterOnDesktop() {
  const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
  if (desktop.size.x > m_Data.Width && desktop.size.y > m_Data.Height) {
    const int posX = static_cast<int>((desktop.size.x - m_Data.Width) / 2);
    const int posY = static_cast<int>((desktop.size.y - m_Data.Height) / 2);
    m_Window.setPosition({posX, posY});
  }
}

void Window::Shutdown() {
  if (m_Window.isOpen()) {
    m_Window.close();
  }
}

void Window::SetVSync(bool enabled) {
  m_Data.VSync = enabled;
  m_Window.setVerticalSyncEnabled(enabled);
}

void Window::Display() { m_Window.display(); }

void Window::Clear(const sf::Color &color) { m_Window.clear(color); }

void Window::OnUpdate() {
  Input::Update();

  while (const std::optional event = m_Window.pollEvent()) {
    ImGui::SFML::ProcessEvent(m_Window, *event);
    Input::HandleEvent(*event);

    if (!m_Data.EventCallback)
      continue;

    if (event->is<sf::Event::Closed>()) {
      WindowCloseEvent e;
      m_Data.EventCallback(e);
    } else if (const auto *resized = event->getIf<sf::Event::Resized>()) {
      m_Data.Width = resized->size.x;
      m_Data.Height = resized->size.y;
      WindowResizeEvent e(resized->size.x, resized->size.y);
      m_Data.EventCallback(e);
    } else if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>()) {
      KeyPressedEvent e(keyPressed->code, 0);
      m_Data.EventCallback(e);
    } else if (const auto *keyReleased =
                   event->getIf<sf::Event::KeyReleased>()) {
      KeyReleasedEvent e(keyReleased->code);
      m_Data.EventCallback(e);
    } else if (const auto *textEntered =
                   event->getIf<sf::Event::TextEntered>()) {
      KeyTypedEvent e(textEntered->unicode);
      m_Data.EventCallback(e);
    } else if (const auto *mbPressed =
                   event->getIf<sf::Event::MouseButtonPressed>()) {
      MouseButtonPressedEvent e(mbPressed->button);
      m_Data.EventCallback(e);
    } else if (const auto *mbReleased =
                   event->getIf<sf::Event::MouseButtonReleased>()) {
      MouseButtonReleasedEvent e(mbReleased->button);
      m_Data.EventCallback(e);
    } else if (const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
      MouseMovedEvent e(static_cast<float>(mouseMoved->position.x),
                        static_cast<float>(mouseMoved->position.y));
      m_Data.EventCallback(e);
    } else if (const auto *mouseScrolled =
                   event->getIf<sf::Event::MouseWheelScrolled>()) {
      float xOffset = 0.0f;
      float yOffset = 0.0f;
      if (mouseScrolled->wheel == sf::Mouse::Wheel::Vertical) {
        yOffset = mouseScrolled->delta;
      } else if (mouseScrolled->wheel == sf::Mouse::Wheel::Horizontal) {
        xOffset = mouseScrolled->delta;
      }
      MouseScrolledEvent e(xOffset, yOffset);
      m_Data.EventCallback(e);
    }
  }
}

} // namespace Elysium
