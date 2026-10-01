#ifndef KEYEVENT_HPP
#define KEYEVENT_HPP

#include "Event.hpp"
#include "KeyCodes.hpp"

namespace Elysium {

class KeyEvent : public Event {
public:
  KeyCode GetKeyCode() const { return m_KeyCode; }

  EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

protected:
  KeyEvent(const KeyCode keycode) : m_KeyCode(keycode) {}

  KeyCode m_KeyCode;
};

class KeyPressedEvent : public KeyEvent {
public:
  KeyPressedEvent(const KeyCode keycode, const int repeatCount)
      : KeyEvent(keycode), m_RepeatCount(repeatCount) {}

  int GetRepeatCount() const { return m_RepeatCount; }

  std::string ToString() const override {
    std::stringstream ss;
    ss << "KeyPressedEvent: " << static_cast<int>(m_KeyCode) << " ("
       << m_RepeatCount << " repeats)";
    return ss.str();
  }

  EVENT_CLASS_TYPE(KeyPressed)

private:
  int m_RepeatCount;
};

class KeyReleasedEvent : public KeyEvent {
public:
  KeyReleasedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

  std::string ToString() const override {
    std::stringstream ss;
    ss << "KeyReleasedEvent: " << static_cast<int>(m_KeyCode);
    return ss.str();
  }

  EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public Event {
public:
  KeyTypedEvent(const uint32_t character) : m_Character(character) {}

  uint32_t GetCharacter() const { return m_Character; }

  std::string ToString() const override {
    std::stringstream ss;
    ss << "KeyTypedEvent: " << m_Character;
    return ss.str();
  }

  EVENT_CLASS_TYPE(KeyTyped)
  EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

private:
  uint32_t m_Character;
};

} // namespace Elysium

#endif // KEYEVENT_HPP
