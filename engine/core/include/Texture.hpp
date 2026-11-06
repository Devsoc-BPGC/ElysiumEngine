#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <SFML/Graphics/Texture.hpp>

class Texture {
public:
  Texture()
      : m_texture(sf::Texture()) {}
  Texture(const char* filename)
      : m_texture(filename) {}
  Texture(const sf::Texture &texture)
      : m_texture(texture) {}
  
  const sf::Texture &GetTexture () const { return m_texture; }
private:
  sf::Texture m_texture;
};

#endif