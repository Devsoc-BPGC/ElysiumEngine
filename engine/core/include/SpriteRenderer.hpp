#ifndef SPRITERENDERER_HPP
#define SPRITERENDERER_HPP

#include "Component.hpp"
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

class SpriteRenderer : public Component {
public:
  sf::Color color = sf::Color::White;
  sf::Vector2f size = {1.0f, 1.0f}; // size in meters
  float radius = 0.5f;
  bool isCircle = false;

  SpriteRenderer() = default;
  SpriteRenderer(sf::Color col, sf::Vector2f sz)
      : color(col), size(sz), radius(0.5f), isCircle(false) {}
  SpriteRenderer(sf::Color col, float r)
      : color(col), radius(r), isCircle(true) {}
};

#endif // SPRITERENDERER_HPP
