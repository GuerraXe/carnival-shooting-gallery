#include "gallery/view/Viewport.hpp"

#include <algorithm>
#include <cmath>

namespace gallery {

Viewport::Viewport() noexcept
    : Viewport(kLogicalSize, kLogicalSize, 1.0f, Vec2{0.0f, 0.0f}) {}

Viewport::Viewport(Size window, Size logical, float scale, Vec2 offset) noexcept
    : window_(window), logical_(logical), scale_(scale), offset_(offset) {}

Viewport Viewport::fit(Size window, Size logical) noexcept {
    if (!(window.width > 0.0f) || !(window.height > 0.0f) ||
        !(logical.width > 0.0f) || !(logical.height > 0.0f)) {
        return Viewport(window, logical, 0.0f, Vec2{0.0f, 0.0f});
    }
    const float scale = std::min(window.width / logical.width, window.height / logical.height);
    const Vec2 offset{(window.width - logical.width * scale) * 0.5f,
                      (window.height - logical.height * scale) * 0.5f};
    return Viewport(window, logical, scale, offset);
}

Rect Viewport::pixelRect() const noexcept {
    return Rect{offset_.x, offset_.y, logical_.width * scale_, logical_.height * scale_};
}

Vec2 Viewport::toLogical(Vec2 windowPx) const noexcept {
    if (degenerate()) {
        return Vec2{0.0f, 0.0f};
    }
    return (windowPx - offset_) / scale_;
}

Vec2 Viewport::toPixel(Vec2 logicalPt) const noexcept {
    return logicalPt * scale_ + offset_;
}

Vec2 Viewport::clampToLogical(Vec2 p) const noexcept {
    // Half-open upper bound: the largest float strictly below the edge.
    const float maxX = std::nextafter(logical_.width, 0.0f);
    const float maxY = std::nextafter(logical_.height, 0.0f);
    return Vec2{std::clamp(p.x, 0.0f, maxX), std::clamp(p.y, 0.0f, maxY)};
}

Size initialWindowSize(Size desktop, float dpiScale, Size preferred, float maxFraction) noexcept {
    const float dpi = dpiScale > 0.0f ? dpiScale : 1.0f;   // also rejects NaN
    float w = preferred.width * dpi;
    float h = preferred.height * dpi;

    if (desktop.width > 0.0f && desktop.height > 0.0f && maxFraction > 0.0f) {
        const float limitW = desktop.width * maxFraction;
        const float limitH = desktop.height * maxFraction;
        const float shrink = std::min({1.0f, limitW / w, limitH / h});
        w *= shrink;
        h *= shrink;
    }
    return Size{std::max(1.0f, std::floor(w)), std::max(1.0f, std::floor(h))};
}

} // namespace gallery
