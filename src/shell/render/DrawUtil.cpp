#include "shell/render/DrawUtil.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/String.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace shell::render {

void drawPlaced(sf::RenderTarget& target, const sf::Texture& texture,
                const gallery::Placement& p, sf::Color tint, const sf::RenderStates& states) {
    if (p.source.width <= 0.0f || p.source.height <= 0.0f) {
        return;
    }
    // Integer texel crop; the scale is recomputed from the rounded crop so the
    // image is never stretched by rounding.
    const sf::IntRect src({static_cast<int>(std::lround(p.source.x)),
                           static_cast<int>(std::lround(p.source.y))},
                          {std::max(1, static_cast<int>(std::lround(p.source.width))),
                           std::max(1, static_cast<int>(std::lround(p.source.height)))});
    sf::Sprite sprite(texture, src);
    const float scale = p.dest.width / static_cast<float>(src.size.x);
    sprite.setScale({scale, scale});
    sprite.setPosition({p.dest.x, p.dest.y});
    sprite.setColor(tint);
    target.draw(sprite, states);
}

sf::Text crispText(const sf::Font& font, std::string_view utf8, float logicalSize,
                   float pixelScale) {
    const float s = pixelScale > 0.0f ? pixelScale : 1.0f;
    const auto px = static_cast<unsigned>(std::max(6.0f, std::round(logicalSize * s)));
    sf::Text text(font, sf::String::fromUtf8(utf8.begin(), utf8.end()), px);
    text.setScale({1.0f / s, 1.0f / s});
    return text;
}

void outline(sf::RenderTarget& target, gallery::Rect r, sf::Color colour, float thickness) {
    sf::RectangleShape shape({r.width, r.height});
    shape.setPosition({r.x, r.y});
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(colour);
    shape.setOutlineThickness(-thickness);
    target.draw(shape);
}

void fillRect(sf::RenderTarget& target, gallery::Rect r, sf::Color colour,
              const sf::RenderStates& states) {
    sf::RectangleShape shape({r.width, r.height});
    shape.setPosition({r.x, r.y});
    shape.setFillColor(colour);
    target.draw(shape, states);
}

} // namespace shell::render
