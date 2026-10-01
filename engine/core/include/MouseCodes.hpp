#ifndef MOUSECODES_HPP
#define MOUSECODES_HPP

#include <SFML/Window/Mouse.hpp>
#include <cstdint>

namespace Elysium {

using MouseCode = sf::Mouse::Button;

namespace Mouse {
enum : uint32_t {
  Left = static_cast<uint32_t>(sf::Mouse::Button::Left),
  Right = static_cast<uint32_t>(sf::Mouse::Button::Right),
  Middle = static_cast<uint32_t>(sf::Mouse::Button::Middle),
  Extra1 = static_cast<uint32_t>(sf::Mouse::Button::Extra1),
  Extra2 = static_cast<uint32_t>(sf::Mouse::Button::Extra2),

  ButtonLast = Extra2,
  ButtonLeft = Left,
  ButtonRight = Right,
  ButtonMiddle = Middle
};
} // namespace Mouse

} // namespace Elysium

#endif // MOUSECODES_HPP
