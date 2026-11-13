#pragma once

#include "gallery/core/Geometry.hpp"

namespace gallery {

// The fixed logical canvas all gameplay and layout use (D-006).
inline constexpr Size kLogicalSize{1920.0f, 1080.0f};

// Letterbox mapping between window pixels and the logical canvas.
//
// This is the ONLY window <-> logical conversion in the game (D-006); the
// shell must not use SFML's mapPixelToCoords for gameplay.
//
//   scale    = min(window.w / logical.w, window.h / logical.h)
//   offset   = centred remainder (the black bars)
//   toLogical(px) = (px - offset) / scale
//   toPixel(l)    = l * scale + offset
//
// A window with zero width or height (e.g. minimised) yields a *degenerate*
// viewport: scale 0, empty pixel rect. toLogical() then returns the logical
// origin instead of dividing by zero; callers should skip rendering.
class Viewport {
public:
    // Identity-sized viewport (window == logical size). Exists so owners can
    // default-construct before the real window size is known.
    Viewport() noexcept;

    [[nodiscard]] static Viewport fit(Size window, Size logical = kLogicalSize) noexcept;

    [[nodiscard]] Rect pixelRect() const noexcept;   // where the canvas lands in the window
    [[nodiscard]] float scale() const noexcept { return scale_; }
    [[nodiscard]] Size window() const noexcept { return window_; }
    [[nodiscard]] Size logical() const noexcept { return logical_; }
    [[nodiscard]] bool degenerate() const noexcept { return scale_ <= 0.0f; }

    [[nodiscard]] Vec2 toLogical(Vec2 windowPx) const noexcept;
    [[nodiscard]] Vec2 toPixel(Vec2 logicalPt) const noexcept;

    // Clamp a logical point into the half-open canvas [0,w) × [0,h) (D-015:
    // aim in the letterbox bars sticks to the nearest canvas edge).
    [[nodiscard]] Vec2 clampToLogical(Vec2 logicalPt) const noexcept;

private:
    Viewport(Size window, Size logical, float scale, Vec2 offset) noexcept;

    Size window_;
    Size logical_;
    float scale_;
    Vec2 offset_;
};

// Initial window client size in physical pixels (risk R-6).
//
// `preferred` is the size at 100 % scaling (96 DPI); it is multiplied by
// `dpiScale` (e.g. 2.5 at 240 DPI) so the window has the same apparent size on
// high-DPI displays, then shrunk uniformly (keeping its aspect ratio) to fit
// within `maxFraction` of the desktop. Results are whole pixels, at least 1.
// Non-positive or NaN inputs fall back to scale 1 / the preferred size.
[[nodiscard]] Size initialWindowSize(Size desktop, float dpiScale,
                                     Size preferred = {1280.0f, 720.0f},
                                     float maxFraction = 0.85f) noexcept;

} // namespace gallery
