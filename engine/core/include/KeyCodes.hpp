#ifndef KEYCODES_HPP
#define KEYCODES_HPP

#include <SFML/Window/Keyboard.hpp>
#include <cstdint>

namespace Elysium {

using KeyCode = sf::Keyboard::Key;

namespace Key {
enum : uint32_t {
  Unknown = static_cast<uint32_t>(sf::Keyboard::Key::Unknown),
  A = static_cast<uint32_t>(sf::Keyboard::Key::A),
  B = static_cast<uint32_t>(sf::Keyboard::Key::B),
  C = static_cast<uint32_t>(sf::Keyboard::Key::C),
  D = static_cast<uint32_t>(sf::Keyboard::Key::D),
  E = static_cast<uint32_t>(sf::Keyboard::Key::E),
  F = static_cast<uint32_t>(sf::Keyboard::Key::F),
  G = static_cast<uint32_t>(sf::Keyboard::Key::G),
  H = static_cast<uint32_t>(sf::Keyboard::Key::H),
  I = static_cast<uint32_t>(sf::Keyboard::Key::I),
  J = static_cast<uint32_t>(sf::Keyboard::Key::J),
  K = static_cast<uint32_t>(sf::Keyboard::Key::K),
  L = static_cast<uint32_t>(sf::Keyboard::Key::L),
  M = static_cast<uint32_t>(sf::Keyboard::Key::M),
  N = static_cast<uint32_t>(sf::Keyboard::Key::N),
  O = static_cast<uint32_t>(sf::Keyboard::Key::O),
  P = static_cast<uint32_t>(sf::Keyboard::Key::P),
  Q = static_cast<uint32_t>(sf::Keyboard::Key::Q),
  R = static_cast<uint32_t>(sf::Keyboard::Key::R),
  S = static_cast<uint32_t>(sf::Keyboard::Key::S),
  T = static_cast<uint32_t>(sf::Keyboard::Key::T),
  U = static_cast<uint32_t>(sf::Keyboard::Key::U),
  V = static_cast<uint32_t>(sf::Keyboard::Key::V),
  W = static_cast<uint32_t>(sf::Keyboard::Key::W),
  X = static_cast<uint32_t>(sf::Keyboard::Key::X),
  Y = static_cast<uint32_t>(sf::Keyboard::Key::Y),
  Z = static_cast<uint32_t>(sf::Keyboard::Key::Z),
  Num0 = static_cast<uint32_t>(sf::Keyboard::Key::Num0),
  Num1 = static_cast<uint32_t>(sf::Keyboard::Key::Num1),
  Num2 = static_cast<uint32_t>(sf::Keyboard::Key::Num2),
  Num3 = static_cast<uint32_t>(sf::Keyboard::Key::Num3),
  Num4 = static_cast<uint32_t>(sf::Keyboard::Key::Num4),
  Num5 = static_cast<uint32_t>(sf::Keyboard::Key::Num5),
  Num6 = static_cast<uint32_t>(sf::Keyboard::Key::Num6),
  Num7 = static_cast<uint32_t>(sf::Keyboard::Key::Num7),
  Num8 = static_cast<uint32_t>(sf::Keyboard::Key::Num8),
  Num9 = static_cast<uint32_t>(sf::Keyboard::Key::Num9),
  Escape = static_cast<uint32_t>(sf::Keyboard::Key::Escape),
  LControl = static_cast<uint32_t>(sf::Keyboard::Key::LControl),
  LShift = static_cast<uint32_t>(sf::Keyboard::Key::LShift),
  LAlt = static_cast<uint32_t>(sf::Keyboard::Key::LAlt),
  RControl = static_cast<uint32_t>(sf::Keyboard::Key::RControl),
  RShift = static_cast<uint32_t>(sf::Keyboard::Key::RShift),
  RAlt = static_cast<uint32_t>(sf::Keyboard::Key::RAlt),
  Space = static_cast<uint32_t>(sf::Keyboard::Key::Space),
  Enter = static_cast<uint32_t>(sf::Keyboard::Key::Enter),
  Backspace = static_cast<uint32_t>(sf::Keyboard::Key::Backspace),
  Tab = static_cast<uint32_t>(sf::Keyboard::Key::Tab),
  Delete = static_cast<uint32_t>(sf::Keyboard::Key::Delete),
  Left = static_cast<uint32_t>(sf::Keyboard::Key::Left),
  Right = static_cast<uint32_t>(sf::Keyboard::Key::Right),
  Up = static_cast<uint32_t>(sf::Keyboard::Key::Up),
  Down = static_cast<uint32_t>(sf::Keyboard::Key::Down),
  F1 = static_cast<uint32_t>(sf::Keyboard::Key::F1),
  F2 = static_cast<uint32_t>(sf::Keyboard::Key::F2),
  F3 = static_cast<uint32_t>(sf::Keyboard::Key::F3),
  F4 = static_cast<uint32_t>(sf::Keyboard::Key::F4),
  F5 = static_cast<uint32_t>(sf::Keyboard::Key::F5),
  F6 = static_cast<uint32_t>(sf::Keyboard::Key::F6),
  F7 = static_cast<uint32_t>(sf::Keyboard::Key::F7),
  F8 = static_cast<uint32_t>(sf::Keyboard::Key::F8),
  F9 = static_cast<uint32_t>(sf::Keyboard::Key::F9),
  F10 = static_cast<uint32_t>(sf::Keyboard::Key::F10),
  F11 = static_cast<uint32_t>(sf::Keyboard::Key::F11),
  F12 = static_cast<uint32_t>(sf::Keyboard::Key::F12)
};
} // namespace Key

} // namespace Elysium

#endif // KEYCODES_HPP
