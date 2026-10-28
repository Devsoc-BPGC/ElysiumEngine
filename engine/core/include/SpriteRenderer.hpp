#ifndef SPRITERENDERER_HPP
#define SPRITERENDERER_HPP

#include "Component.hpp"
#include "Texture.hpp"
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

class SpriteRenderer : public Component {
public:
  std::shared_ptr<const Texture> texture = nullptr;
  sf::Color color = sf::Color::White;
  sf::Vector2f size = {1.0f, 1.0f}; // size in meters
  float radius = 0.5f;
  bool isCircle = false;
  bool isTextured = false;
  uint64_t textureHandle = 0;

  SpriteRenderer() = default;
  SpriteRenderer(sf::Color col, sf::Vector2f sz)
      : color(col), size(sz), radius(0.5f), isCircle(false), isTextured(false) {}
  SpriteRenderer(sf::Color col, float r)
      : color(col), radius(r), isCircle(true), isTextured(false) {}
  SpriteRenderer(std::shared_ptr<const Texture> tex, sf::Color col = sf::Color::White, sf::Vector2f sz = sf::Vector2f(1.0f, 1.0f))
      : texture(tex), color(col), size(sz), isCircle(false), isTextured(true) {}

  void SetTextureHandle(uint64_t handle) {
    textureHandle = handle;
    if (handle == 0) {
      isTextured = false;
      texture = nullptr;
    }
  }

  uint64_t GetTextureHandle() const {
    return textureHandle;
  }
};

#endif // SPRITERENDERER_HPP
