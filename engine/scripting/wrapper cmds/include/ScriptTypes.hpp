#pragma once

#include "CoreMath.hpp"
#include <cstdint>
#include <cmath>

namespace Elysium {

using Vector2 = ::Vec2;
using Vec2 = ::Vec2;

/**
 * @struct ColorRGBA
 * @brief Represents an 8-bit RGBA color.
 */
struct ColorRGBA {
    uint8_t r = 255;
    uint8_t green = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    ColorRGBA() = default;
    ColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), green(g), b(b), a(a) {}

    static ColorRGBA Red()     { return {240, 80, 80}; }
    static ColorRGBA Green()   { return {80, 200, 120}; }
    static ColorRGBA Blue()    { return {80, 160, 240}; }
    static ColorRGBA Yellow()  { return {250, 220, 80}; }
    static ColorRGBA Purple()  { return {170, 100, 230}; }
    static ColorRGBA Orange()  { return {245, 140, 60}; }
    static ColorRGBA Cyan()    { return {60, 210, 230}; }
    static ColorRGBA Magenta() { return {230, 80, 190}; }
    static ColorRGBA White()   { return {255, 255, 255}; }
    static ColorRGBA Black()   { return {0, 0, 0}; }
    static ColorRGBA Gray()    { return {128, 128, 128}; }
};

} // namespace Elysium
