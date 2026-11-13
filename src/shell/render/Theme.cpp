#include "shell/render/Theme.hpp"

#include "shell/render/DrawUtil.hpp"

#include <SFML/Graphics/Text.hpp>

namespace shell::render::theme {

namespace {
sf::Text make(const sf::Font& font, std::string_view utf8, float size, float pixelScale,
              float letterSpacing, bool bold) {
    sf::Text text = crispText(font, utf8, size, pixelScale);
    text.setLetterSpacing(letterSpacing);
    if (bold) {
        text.setStyle(sf::Text::Bold);
    }
    return text;
}
} // namespace

gallery::Rect letter(sf::RenderTarget& t, const sf::Font& font, std::string_view utf8, float size,
                     gallery::Vec2 at, Align align, sf::Color colour, float pixelScale,
                     float letterSpacing, bool bold) {
    sf::Text text = make(font, utf8, size, pixelScale, letterSpacing, bold);
    const sf::FloatRect b = text.getLocalBounds();   // glyph box in unscaled (pixel) units
    float ox = b.position.x;
    if (align == Align::Centre) {
        ox += b.size.x / 2.0f;
    } else if (align == Align::Right) {
        ox += b.size.x;
    }
    text.setOrigin({ox, b.position.y});
    text.setPosition({at.x, at.y});
    text.setFillColor(colour);
    t.draw(text);
    const sf::FloatRect g = text.getGlobalBounds();
    return {g.position.x, g.position.y, g.size.x, g.size.y};
}

float letterWidth(const sf::Font& font, std::string_view utf8, float size, float pixelScale,
                  float letterSpacing, bool bold) {
    const sf::Text text = make(font, utf8, size, pixelScale, letterSpacing, bold);
    return text.getGlobalBounds().size.x;
}

void rule(sf::RenderTarget& t, float centreX, float y, float width, sf::Color colour,
          float thickness) {
    fillRect(t, {centreX - width / 2.0f, y - thickness / 2.0f, width, thickness}, colour);
}

} // namespace shell::render::theme
