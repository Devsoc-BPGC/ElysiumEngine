#ifndef INPUT_HPP
#define INPUT_HPP

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Event.hpp>
#include <unordered_map>

class Input {
public:
    enum class ButtonState {
        None,
        Pressed,
        Held,
        Released
    };

    static void Update() {
        m_mouseWheelDelta = 0.0f;
        // Transition Pressed to Held, and Released to None
        for (auto& [key, state] : m_keyStates) {
            if (state == ButtonState::Pressed) state = ButtonState::Held;
            else if (state == ButtonState::Released) state = ButtonState::None;
        }
        for (auto& [button, state] : m_mouseButtonStates) {
            if (state == ButtonState::Pressed) state = ButtonState::Held;
            else if (state == ButtonState::Released) state = ButtonState::None;
        }
    }

    static void HandleEvent(const sf::Event& event) {
        if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            if (m_keyStates[keyPressed->code] != ButtonState::Held) {
                m_keyStates[keyPressed->code] = ButtonState::Pressed;
            }
        }
        else if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
            m_keyStates[keyReleased->code] = ButtonState::Released;
        }
        else if (const auto* mbPressed = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (m_mouseButtonStates[mbPressed->button] != ButtonState::Held) {
                m_mouseButtonStates[mbPressed->button] = ButtonState::Pressed;
            }
        }
        else if (const auto* mbReleased = event.getIf<sf::Event::MouseButtonReleased>()) {
            m_mouseButtonStates[mbReleased->button] = ButtonState::Released;
        }
        else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
            m_mousePosition = { (float)mouseMoved->position.x, (float)mouseMoved->position.y };
        }
        else if (const auto* mouseWheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
            if (mouseWheel->wheel == sf::Mouse::Wheel::Vertical) {
                m_mouseWheelDelta = mouseWheel->delta;
            }
        }
    }

    static bool IsKeyDown(sf::Keyboard::Key key) {
        auto it = m_keyStates.find(key);
        return it != m_keyStates.end() && (it->second == ButtonState::Pressed || it->second == ButtonState::Held);
    }

    static bool IsKeyPressed(sf::Keyboard::Key key) {
        auto it = m_keyStates.find(key);
        return it != m_keyStates.end() && it->second == ButtonState::Pressed;
    }

    static bool IsKeyReleased(sf::Keyboard::Key key) {
        auto it = m_keyStates.find(key);
        return it != m_keyStates.end() && it->second == ButtonState::Released;
    }

    static bool IsMouseButtonDown(sf::Mouse::Button button) {
        auto it = m_mouseButtonStates.find(button);
        return it != m_mouseButtonStates.end() && (it->second == ButtonState::Pressed || it->second == ButtonState::Held);
    }

    static bool IsMouseButtonPressed(sf::Mouse::Button button) {
        auto it = m_mouseButtonStates.find(button);
        return it != m_mouseButtonStates.end() && it->second == ButtonState::Pressed;
    }

    static bool IsMouseButtonReleased(sf::Mouse::Button button) {
        auto it = m_mouseButtonStates.find(button);
        return it != m_mouseButtonStates.end() && it->second == ButtonState::Released;
    }

    static sf::Vector2f GetMousePosition() { return m_mousePosition; }
    static float GetMouseWheelDelta() { return m_mouseWheelDelta; }

private:
    static inline std::unordered_map<sf::Keyboard::Key, ButtonState> m_keyStates;
    static inline std::unordered_map<sf::Mouse::Button, ButtonState> m_mouseButtonStates;
    static inline sf::Vector2f m_mousePosition;
    static inline float m_mouseWheelDelta = 0.0f;
};

#endif
