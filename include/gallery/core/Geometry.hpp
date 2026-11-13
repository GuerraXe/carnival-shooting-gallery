#pragma once

// Plain value types for the logic layer. Deliberately separate from
// sf::Vector2f / sf::FloatRect so gallery_logic stays SFML-free (D-005);
// conversions live in the shell.

namespace gallery {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    friend constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept { return {a.x + b.x, a.y + b.y}; }
    friend constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept { return {a.x - b.x, a.y - b.y}; }
    friend constexpr Vec2 operator*(Vec2 a, float s) noexcept { return {a.x * s, a.y * s}; }
    friend constexpr Vec2 operator/(Vec2 a, float s) noexcept { return {a.x / s, a.y / s}; }
    friend constexpr bool operator==(Vec2, Vec2) noexcept = default;
};

// A width/height pair. Used for window sizes (pixels) and the logical canvas.
struct Size {
    float width = 0.0f;
    float height = 0.0f;

    friend constexpr bool operator==(Size, Size) noexcept = default;
};

// Axis-aligned rectangle, half-open: [x, x+width) × [y, y+height).
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    [[nodiscard]] constexpr bool contains(Vec2 p) const noexcept {
        return p.x >= x && p.x < x + width && p.y >= y && p.y < y + height;
    }
    friend constexpr bool operator==(Rect, Rect) noexcept = default;
};

} // namespace gallery
