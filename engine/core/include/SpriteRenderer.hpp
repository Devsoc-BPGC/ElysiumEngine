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
  uint64_t textureHandle = 0; // 0 = unassigned / solid color

  SpriteRenderer() = default;
  SpriteRenderer(sf::Color col, sf::Vector2f sz)
      : color(col), size(sz), radius(0.5f), isCircle(false), textureHandle(0) {}
  SpriteRenderer(sf::Color col, float r)
      : color(col), radius(r), isCircle(true), textureHandle(0) {}
  SpriteRenderer(uint64_t texHandle, sf::Vector2f sz)
      : color(sf::Color::White), size(sz), radius(0.5f), isCircle(false), textureHandle(texHandle) {}

  std::unique_ptr<Component> Clone() const override {
    return std::make_unique<SpriteRenderer>(*this);
  }
};

#endif // SPRITERENDERER_HPP
