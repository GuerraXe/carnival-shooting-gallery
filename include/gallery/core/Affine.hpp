#pragma once

// 2D affine transform for the logic layer (ARCHITECTURE §8: "Vec2 and an
// affine transform are about 100 lines"). Same matrix layout as sf::Transform
// so the shell can convert it 1:1:
//
//   | a  b  tx |   x' = a·x + b·y + tx
//   | c  d  ty |   y' = c·x + d·y + ty
//
// Used for the duck pose (risk R-3): the renderer draws a duck through
// exactly the transform that hit testing inverts.

#include "gallery/core/Geometry.hpp"

#include <cmath>
#include <optional>

namespace gallery {

struct Affine2 {
    float a = 1.0f, b = 0.0f, tx = 0.0f;
    float c = 0.0f, d = 1.0f, ty = 0.0f;

    [[nodiscard]] constexpr Vec2 apply(Vec2 p) const noexcept {
        return {a * p.x + b * p.y + tx, c * p.x + d * p.y + ty};
    }

    // this ∘ rhs: apply rhs first, then this.
    [[nodiscard]] constexpr Affine2 operator*(const Affine2& r) const noexcept {
        return {a * r.a + b * r.c, a * r.b + b * r.d, a * r.tx + b * r.ty + tx,
                c * r.a + d * r.c, c * r.b + d * r.d, c * r.tx + d * r.ty + ty};
    }

    // nullopt when the transform collapses an axis (e.g. a duck lying flat).
    [[nodiscard]] std::optional<Affine2> inverse(float minDet = 1e-6f) const noexcept {
        const float det = a * d - b * c;
        if (!std::isfinite(det) || std::fabs(det) < minDet) {
            return std::nullopt;
        }
        const float ia = d / det, ib = -b / det, ic = -c / det, id = a / det;
        return Affine2{ia, ib, -(ia * tx + ib * ty), ic, id, -(ic * tx + id * ty)};
    }

    [[nodiscard]] static constexpr Affine2 translation(Vec2 t) noexcept {
        return {1.0f, 0.0f, t.x, 0.0f, 1.0f, t.y};
    }
    [[nodiscard]] static constexpr Affine2 scaling(float sx, float sy) noexcept {
        return {sx, 0.0f, 0.0f, 0.0f, sy, 0.0f};
    }
    // Clockwise-positive in a y-down frame (the SFML convention).
    [[nodiscard]] static Affine2 rotationDeg(float deg) noexcept {
        const float r = deg * 3.14159265358979f / 180.0f;
        const float cs = std::cos(r), sn = std::sin(r);
        return {cs, -sn, 0.0f, sn, cs, 0.0f};
    }
};

} // namespace gallery
