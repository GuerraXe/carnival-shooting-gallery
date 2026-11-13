#pragma once

// Small SFML helpers shared by the render passes. Shell only.

#include "gallery/core/Affine.hpp"
#include "gallery/core/Geometry.hpp"
#include "gallery/view/Placement.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transform.hpp>

#include <string_view>

namespace shell::render {

[[nodiscard]] inline sf::Vector2f toSf(gallery::Vec2 v) noexcept { return {v.x, v.y}; }
[[nodiscard]] inline sf::FloatRect toSf(gallery::Rect r) noexcept {
    return {{r.x, r.y}, {r.width, r.height}};
}
[[nodiscard]] inline sf::Transform toSf(const gallery::Affine2& m) noexcept {
    return {m.a, m.b, m.tx, m.c, m.d, m.ty, 0.0f, 0.0f, 1.0f};
}
[[nodiscard]] inline gallery::Size sizeOf(const sf::Texture& t) noexcept {
    return {static_cast<float>(t.getSize().x), static_cast<float>(t.getSize().y)};
}

[[nodiscard]] inline sf::Color withAlpha(sf::Color c, float alpha01) noexcept {
    const float a = alpha01 < 0.0f ? 0.0f : (alpha01 > 1.0f ? 1.0f : alpha01);
    c.a = static_cast<std::uint8_t>(static_cast<float>(c.a) * a + 0.5f);
    return c;
}

// Draw `texture` according to a Placement (uniform scale, optional crop).
void drawPlaced(sf::RenderTarget& target, const sf::Texture& texture,
                const gallery::Placement& placement, sf::Color tint = sf::Color::White,
                const sf::RenderStates& states = sf::RenderStates::Default);

// Text whose glyphs are rasterised at the target's real pixel size, then
// scaled back into logical units, so it is crisp at any window size.
// `pixelScale` = physical pixels per logical unit (Viewport::scale()).
[[nodiscard]] sf::Text crispText(const sf::Font& font, std::string_view utf8,
                                 float logicalSize, float pixelScale);

// Outline of a rectangle (inspection overlay only; never part of the scene).
void outline(sf::RenderTarget& target, gallery::Rect r, sf::Color colour, float thickness);

void fillRect(sf::RenderTarget& target, gallery::Rect r, sf::Color colour,
              const sf::RenderStates& states = sf::RenderStates::Default);

} // namespace shell::render
